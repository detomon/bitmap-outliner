#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 -lqrencode && /tmp/`basename $0` $@; exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Arrow types.
 */
typedef enum {
	ARROW_NONE = 0,
	ARROW_RIGHT,
	ARROW_LEFT,
	ARROW_DOWN,
	ARROW_UP
} arrow_type;

/**
 * Information about how to proceed to next arrow.
 */
typedef struct {
	arrow_type arrow; ///< Type of arrow.
	int8_t dx;        ///< Relative to current position.
	int8_t dy;        ///< Relative to current position.
} arrow_next;

/**
 * Defines arrow.
 */
typedef struct {
	uint8_t type:3;  ///< Arrow type.
	uint8_t inner:1; ///< Associated path is inner path.
	uint8_t seen:1;  ///< If already seen.
} grid_arrow;

/**
 * Defines path segment.
 */
typedef struct {
	arrow_type type; ///< Arrow type.
	int dx;          ///< Path segment width.
	int dy;          ///< Path segment height.
} path_segment;

/**
 * The arrow states.
 */
static arrow_next const states[][2][3] = {
	[ARROW_RIGHT] = {
		{{ARROW_UP,   +1, -1}, {ARROW_RIGHT, +1,  0}, {ARROW_DOWN, +1, +1}},
		{{ARROW_DOWN, +1, +1}, {ARROW_RIGHT, +1,  0}, {ARROW_UP,   +1, -1}},
	},
	[ARROW_LEFT] = {
		{{ARROW_DOWN,  0, +1}, {ARROW_LEFT,  -1,  0}, {ARROW_UP,    0, -1}},
		{{ARROW_UP,    0, -1}, {ARROW_LEFT,  -1,  0}, {ARROW_DOWN,  0, +1}},
	},
	[ARROW_DOWN] = {
		{{ARROW_RIGHT, 0, +1}, {ARROW_DOWN,   0, +2}, {ARROW_LEFT, -1, +1}},
		{{ARROW_LEFT, -1, +1}, {ARROW_DOWN,   0, +2}, {ARROW_RIGHT, 0, +1}},
	},
	[ARROW_UP] = {
		{{ARROW_LEFT, -1, -1}, {ARROW_UP,     0, -2}, {ARROW_RIGHT, 0, -1}},
		{{ARROW_RIGHT, 0, -1}, {ARROW_UP,     0, -2}, {ARROW_LEFT, -1, -1}},
	},
};

/**
 * Create arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @return Arrow grid.
 */
static grid_arrow* createArrowGrid(int width, int height) {
	return calloc((width + 1 + 3) * (height * 2 + 3), sizeof(grid_arrow));
}

/**
 * Fill arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param map The bitmap.
 * @param grid Grid to fill with arrows.
 */
static void setArrowsFromMap(int width, int height, uint8_t const map[height][width], grid_arrow grid[height * 2 + 3][width + 3]) {
	int x, y, p, t;

	for (x = 0; x < width; x++) {
		for (y = 0, t = 0; y < height; y++) {
			if ((p = map[y][x]) != t) {
				grid[y * 2 + 1][x + 1].type = t ? ARROW_LEFT : ARROW_RIGHT;
				t = p;
			}
		}

		if (map[y - 1][x]) {
			grid[y * 2 + 1][x + 1].type = ARROW_LEFT;
		}
	}

	for (y = 0; y < height; y++) {
		for (x = 0, t = 0; x < width; x++) {
			if ((p = map[y][x]) != t) {
				grid[y * 2 + 2][x + 1].type = t ? ARROW_DOWN : ARROW_UP;
				t = p;
			}
		}

		if (map[y][x - 1]) {
			grid[y * 2 + 2][x + 1].type = ARROW_DOWN;
		}
	}
}

/**
 * Convert arrow grid coordinates to path coordinates.
 *
 * @param type Arrow type.
 * @param gx Grid X-coordinate.
 * @param gy Grid Y-coordinate.
 * @param rx Path X-coordinates.
 * @param ry Path Y-coordinates.
 */
static void realCoords(arrow_type type, int32_t gx, int32_t gy, int32_t* rx, int32_t* ry) {
	/**
	 * Defines coordinates.
	 */
	typedef struct {
		int x; ///< X-coordinate.
		int y; ///< Y-coordinate.
	} coords;

	/**
	 * Delta to add to convert to real coordinates.
	 */
	static coords const real_delta[] = {
		[ARROW_RIGHT] = {0, 0},
		[ARROW_LEFT]  = {1, 0},
		[ARROW_DOWN]  = {0, 0},
		[ARROW_UP]    = {0, 1},
	};

	coords const* real = &real_delta[type];

	*rx = gx - 1 + real->x;
	*ry = (gy - 1) / 2 + real->y;
}

/**
 * Mark arrow as outer and inner.
 *
 * @param x First path arrow.
 * @param y First path arrow.
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param a First arrow.
 * @param grid Grid to search for paths.
 */
static int makePath(path_segment* segments, int x, int y, int width, int height, grid_arrow grid[height * 2 + 3][width + 3]) {
	int xd = x;
	int yd = y;
	int xr, yr;
	int xp, yp;
	grid_arrow* currentArrow = &grid[yd][xd];
	grid_arrow* nextArrow = currentArrow;
	arrow_type type = currentArrow->type;
	// assume path is inner path
	int inner = (type == ARROW_LEFT);
	arrow_type prevtype = type;
	path_segment* segment;
	int count = 0;

	realCoords(type, xd, yd, &xr, &yr);

	xp = xr;
	yp = yr;

	segment = &segments[count++];
	segment->type = ARROW_NONE;
	segment->dx = xr;
	segment->dy = yr;

	do {
		arrow_next const* arrows = &states[type][inner][0];

		currentArrow = nextArrow;
		type = ARROW_NONE;

		// mark as seen
		currentArrow->seen = 1;
		currentArrow->inner = inner;

		// search for adjacent arrows in precedence order
		for (int n = 0; n < 3; n++) {
			arrow_next const* search = &arrows[n];
			int xn = xd + search->dx;
			int yn = yd + search->dy;

			nextArrow = &grid[yn][xn];

			// follow adjacent arrow
			if (nextArrow->type == search->arrow && !nextArrow->seen) {
				xd = xn;
				yd = yn;
				type = search->arrow;
				break;
			}
		}

		// end path segment if arrow changes
		// and ignore last path segment
		if (type != prevtype && type) {
			int dx, dy;
			realCoords(type, xd, yd, &xr, &yr);

			dx = xr - xp;
			dy = yr - yp;

			segment = &segments[count++];
			segment->type = prevtype;
			segment->dx = dx;
			segment->dy = dy;

			xp = xr;
			yp = yr;
			prevtype = type;
		}
	}
	while (type);

	return count;
}

/**
 * Search all paths in arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Grid to search for paths.
 */
static void searchPaths(int width, int height, grid_arrow grid[height * 2 + 3][width + 3]) {
	int gridWidth = width + 3;
	int gridHeight = height * 2 + 3;

	// TODO: Resize buffer
	path_segment segments[1000];

	// search right and left arrows in grid
	for (int y = 1; y < gridHeight - 1; y += 2) {
		for (int x = 1; x < gridWidth - 1; x++) {
			grid_arrow arrow = grid[y][x];

			if (arrow.type && !arrow.seen) {
				int count = makePath(segments, x, y, width, height, grid);
				path_segment* segment = &segments[0];

				printf("M %d %d", segment->dx, segment->dy);

				for (int i = 1; i < count; i++) {
					segment = &segments[i];

					switch (segment->type) {
						case ARROW_RIGHT:
						case ARROW_LEFT: {
							printf("h%d", segment->dx);
							break;
						}
						case ARROW_DOWN:
						case ARROW_UP: {
							printf("v%d", segment->dy);
							break;
						}
						default: {
							break;
						}
					}
				}

				printf("z");
			}
		}
	}
}

// -----------------------------------------------------------------------------

#include <qrencode.h>

/**
 * Terminal colors.
 */
enum {
	COLOR_RESET = 0,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
};

/**
 * Terminal color escape sequences.
 */
static char const* const colors[] = {
	[COLOR_RESET]  = "\033[0m",
	[COLOR_RED]    = "\033[31m",
	[COLOR_GREEN]  = "\033[32m",
	[COLOR_YELLOW] = "\033[33m",
};

/**
 * Follow path at given position.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param map The bitmap to print.
 * @param grid The arrow grid to print.
 */
static void printGrid(int width, int height, uint8_t const map[height][width], grid_arrow const grid[height * 2 + 3][width + 3]) {
	static const char* arrows[] = {
		[ARROW_NONE]  = "∙",
		[ARROW_RIGHT] = "→",
		[ARROW_LEFT]  = "←",
		[ARROW_DOWN]  = "↓",
		[ARROW_UP]    = "↑",
	};

	int gridWidth = width + 3;
	int gridHeight = height * 2 + 3;

	for (int y = 0; y < gridHeight; y++) {
		if (y % 2 != 0) {
			printf("  ");
		}

		for (int x = 0; x < gridWidth - (y % 2 != 0); x++) {
			grid_arrow a = grid[y][x];
			int type = a.type;
			char const* color = "";

			if (type) {
				color = colors[a.inner ? COLOR_RED : COLOR_GREEN];
			}

			printf("%s%s%s", color, arrows[(int)type], colors[COLOR_RESET]);

			if (x > 0 && y >= 2 && x < gridWidth - 2 && y < gridHeight - 2 && (y % 2) == 0) {
				printf(" %c ", map[(y - 2) / 2][x - 1] ? '#' : ' ');
			}
			else {
				printf("   ");
			}
		}

		printf("\n");
	}
}

#define WIDTH 7
#define HEIGHT 7


char const map[] =
	"\x01\x01\x00\x01\x00\x01\x01"
	"\x00\x00\x01\x00\x01\x00\x00"
	"\x01\x00\x01\x01\x01\x00\x01"
	"\x01\x01\x00\x00\x00\x01\x01"
	"\x01\x01\x00\x00\x00\x01\x01"
	"\x01\x01\x00\x01\x00\x01\x01"
	"\x01\x01\x00\x01\x00\x01\x01"
;

/*
char const map[] =
	"\x01\x00\x01\x00\x01"
	"\x00\x01\x00\x01\x00"
	"\x01\x00\x01\x00\x01"
	"\x00\x01\x00\x01\x00"
	"\x01\x00\x01\x00\x01"
	"\x01\x00\x01\x00\x01"
	"\x00\x01\x00\x01\x00"
	"\x01\x00\x01\x00\x01"
	"\x00\x01\x00\x01\x00"
	"\x01\x00\x01\x00\x01"
;
*/

int main() {
	int width = WIDTH;
	int height = HEIGHT;
	char* data = map;

	QRcode* code = QRcode_encodeString("https://pipeline2017.station.ch/main/index.php", 0, 0, QR_MODE_8, 0);

	width = code->width;
	height = code->width;

	for (int i = 0; i < width * height; i++) {
		code->data[i] &= 1;
	}

	data = code->data;

	grid_arrow* grid = createArrowGrid(width, height);

	setArrowsFromMap(width, height, (void const*)data, (void*)grid);

	printf("<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"", width, height);
	searchPaths(width, height, (void*)grid);
	printf("\"></path></svg>\n");

	printf("\n");
	printGrid(width, height, (void const*)data, (void const*)grid);
	printf("\n");

	free(grid);

	return 0;
}
