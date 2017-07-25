#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 && /tmp/`basename $0` $@; exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 5
#define HEIGHT 10

enum {
	ARROW_NONE = 0,
	ARROW_RIGHT,
	ARROW_LEFT,
	ARROW_DOWN,
	ARROW_UP
};

enum {
	DIR_STRAIGHT = 0,
	DIR_RIGHT,
	DIR_LEFT,
};

enum {
	COLOR_RESET = 0,
	COLOR_RED,
	COLOR_GREEN,
	COLOR_YELLOW,
};

/*
char const map[] =
	"\x01\x01\x00\x01\x00\x01\x01"
	"\x00\x00\x01\x00\x01\x00\x00"
	"\x01\x00\x01\x01\x01\x00\x01"
	"\x01\x01\x00\x00\x00\x01\x01"
	"\x01\x01\x00\x00\x00\x01\x01"
	"\x01\x01\x00\x01\x00\x01\x01"
	"\x01\x01\x00\x01\x00\x01\x01"
;
*/

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


/**
 * Information about how to proceed to next arrow.
 */
struct arrowState {
	int8_t rxd; ///< Delta to add to x to convert to real coordinates.
	int8_t ryd; ///< Delta to add to y to convert to real coordinates.
	struct arrowNext {
		int8_t dir;
		int8_t isA;
		int8_t toDir;

		int8_t dx;      ///< Relative to current position.
		int8_t dy;      ///< Relative to current position.
		uint8_t arrow;  ///< Type of arrow.
		uint8_t inner;  ///< Is arrow of inner path.
	} next[4]; ///< Adjacent arrows.
};

/**
 * Defines arrow.
 */
struct arrow {
	uint8_t type:3;     ///< Arrow type.
	uint8_t inner:1;    ///< Associated path is inner path.
	uint8_t seen:1;     ///< If already seen.
	uint8_t followed:1; ///< If already followed.
};

/**
 * Defines coordinate delta.
 */
struct delta {
	int8_t x;     ///< Delta coordinate.
	int8_t y;     ///< Delta coordinate.
	//uint8_t type; ///< Arrow type.
};

/**
 * Coordinates to arrows relative from current arrow.
 */
struct delta const directions[][3] = {
	[ARROW_RIGHT] = {
		[DIR_STRAIGHT] = {+1,  0/*, ARROW_RIGHT*/},
		[DIR_RIGHT]    = {+1, +1/*, ARROW_DOWN*/},
		[DIR_LEFT]     = {+1, -1/*, ARROW_UP*/},
	},
	[ARROW_LEFT] = {
		[DIR_STRAIGHT] = {-1,  0/*, ARROW_LEFT*/},
		[DIR_RIGHT]    = { 0, -1/*, ARROW_UP*/},
		[DIR_LEFT]     = { 0, +1/*, ARROW_DOWN*/},
	},
	[ARROW_DOWN] = {
		[DIR_STRAIGHT] = { 0, +2/*, ARROW_DOWN*/},
		[DIR_RIGHT]    = {-1, +1/*, ARROW_LEFT*/},
		[DIR_LEFT]     = { 0, +1/*, ARROW_RIGHT*/},
	},
	[ARROW_UP] = {
		[DIR_STRAIGHT] = { 0, -2/*, ARROW_UP*/},
		[DIR_RIGHT]    = { 0, -1/*, ARROW_RIGHT*/},
		[DIR_LEFT]     = {-1, -1/*, ARROW_LEFT*/},
	},
};

/**
 * The arrow states.
 */
static struct arrowState states[][2] = {
	[ARROW_RIGHT] = {
		// outer
		{
			.rxd =  0,
			.ryd =  0,
			.next = {
				{DIR_STRAIGHT, ARROW_LEFT,  DIR_RIGHT,    +1,  0, ARROW_LEFT, 1},
				{DIR_LEFT,     ARROW_UP,    DIR_LEFT,     +1, -1, ARROW_UP, 0},
				{DIR_STRAIGHT, ARROW_RIGHT, DIR_STRAIGHT, +1,  0, ARROW_RIGHT, 0},
				{DIR_RIGHT,    ARROW_DOWN,  DIR_RIGHT,    +1, +1, ARROW_DOWN, 0},
			},
		},
		// inner
		{
			.rxd =  0,
			.ryd =  0,
			.next = {
				{DIR_STRAIGHT, ARROW_LEFT,  DIR_LEFT,     +1,  0, ARROW_LEFT, 0},
				{DIR_RIGHT,    ARROW_DOWN,  DIR_RIGHT,    +1, +1, ARROW_DOWN, 1},
				{DIR_STRAIGHT, ARROW_RIGHT, DIR_STRAIGHT, +1,  0, ARROW_RIGHT, 1},
				{DIR_LEFT,     ARROW_UP,    DIR_LEFT,     +1, -1, ARROW_UP, 1},
			},
		},
	},
	[ARROW_LEFT] = {
		// outer
		{
			.rxd = +1,
			.ryd =  0,
			.next = {
				{DIR_STRAIGHT, ARROW_RIGHT, DIR_RIGHT,    -1,  0, ARROW_RIGHT, 1},
				{DIR_LEFT,     ARROW_DOWN,  DIR_LEFT,      0, +1, ARROW_DOWN, 0},
				{DIR_STRAIGHT, ARROW_LEFT,  DIR_STRAIGHT, -1,  0, ARROW_LEFT, 0},
				{DIR_RIGHT,    ARROW_UP,    DIR_RIGHT,     0, -1, ARROW_UP, 0},
			},
		},
		// inner
		{
			.rxd = +1,
			.ryd =  0,
			.next = {
				{DIR_STRAIGHT, ARROW_RIGHT, DIR_LEFT,     -1,  0, ARROW_RIGHT, 0},
				{DIR_RIGHT,    ARROW_UP,    DIR_RIGHT,     0, -1, ARROW_UP, 1},
				{DIR_STRAIGHT, ARROW_LEFT,  DIR_STRAIGHT, -1,  0, ARROW_LEFT, 1},
				{DIR_LEFT,     ARROW_DOWN,  DIR_LEFT,      0, +1, ARROW_DOWN, 1},
			},
		},
	},
	[ARROW_DOWN] = {
		// outer
		{
			.rxd =  0,
			.ryd =  0,
			.next = {
				{DIR_STRAIGHT, ARROW_UP,    DIR_RIGHT,      0, +2, ARROW_UP, 1},
				{DIR_LEFT,     ARROW_RIGHT, DIR_LEFT,      0, +1, ARROW_RIGHT, 0},
				{DIR_STRAIGHT, ARROW_DOWN,  DIR_STRAIGHT,   0, +2, ARROW_DOWN, 0},
				{DIR_RIGHT,    ARROW_LEFT,  DIR_RIGHT,      -1, +1, ARROW_LEFT, 0},
			},
		},
		// inner
		{
			.rxd =  0,
			.ryd =  0,
			.next = {
				{DIR_STRAIGHT, ARROW_UP,    DIR_LEFT,     0, +2, ARROW_UP, 0},
				{DIR_RIGHT,    ARROW_LEFT,  DIR_RIGHT,    -1, +1, ARROW_LEFT, 1},
				{DIR_STRAIGHT, ARROW_DOWN,  DIR_STRAIGHT, 0, +2, ARROW_DOWN, 1},
				{DIR_LEFT,     ARROW_RIGHT, DIR_LEFT,    0, +1, ARROW_RIGHT, 1},
			},
		},
	},
	[ARROW_UP] = {
		// outer
		{
			.rxd =  0,
			.ryd = +1,
			.next = {
				{DIR_STRAIGHT, ARROW_DOWN,  DIR_RIGHT,    0, -2, ARROW_DOWN, 1},
				{DIR_LEFT,     ARROW_LEFT,  DIR_LEFT,    -1, -1, ARROW_LEFT, 0},
				{DIR_STRAIGHT, ARROW_UP,    DIR_STRAIGHT, 0, -2, ARROW_UP, 0},
				{DIR_RIGHT,    ARROW_RIGHT, DIR_RIGHT,    0, -1, ARROW_RIGHT, 0},
			},
		},
		// inner
		{
			.rxd =  0,
			.ryd = +1,
			.next = {
				{DIR_STRAIGHT, ARROW_DOWN,  DIR_LEFT,      0, -2, ARROW_DOWN, 0},
				{DIR_RIGHT,    ARROW_RIGHT, DIR_RIGHT,     0, -1, ARROW_RIGHT, 1},
				{DIR_STRAIGHT, ARROW_UP,    DIR_STRAIGHT,  0, -2, ARROW_UP, 1},
				{DIR_LEFT,     ARROW_LEFT,  DIR_LEFT,     -1, -1, ARROW_LEFT, 1},
			},
		},
	},
};

/**
 * Terminal colors.
 */
static char const* const colors[] = {
	[COLOR_RESET]  = "\033[0m",
	[COLOR_RED]    = "\033[31m",
	[COLOR_GREEN]  = "\033[32m",
	[COLOR_YELLOW] = "\033[33m",
};

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
static void preparePath(int x, int y, int width, int height, struct arrow a, struct arrow grid[height * 2 + 3][width + 3]) {
	int type = a.type;
	// assume path is inner path
	int inner = (type == ARROW_LEFT);
	// current arrow position
	int xd = x;
	int yd = y;

	// arrow state
	struct arrowState const* arrow = &states[type][inner];
	// real coordinates
	int xr = xd - 1 + arrow->rxd;
	int yr = (yd - 1) / 2 + arrow->ryd;

	struct arrow* nextArrow = &grid[yd][xd];
	struct arrow* currentArrow;

	do {
		currentArrow = nextArrow;
		type = ARROW_NONE;

		// mark as seen
		currentArrow->seen = 1;
		currentArrow->inner = inner;

		// search for adjacent arrows in precedence order
		for (int n = 0; n < 4; n++) {
			// adjacent arrow
			struct arrowNext const* search = &arrow->next[n];

			// TODO: Use directions

			struct delta d = directions[currentArrow->type][search->dir];
			int xn = xd + d.x;
			int yn = yd + d.y;

			//int xn = xd + search->dx;
			//int yn = yd + search->dy;

			/*if (d.x != search->dx || d.y != search->dy) {
				printf("%d %d %d %d %d %d\n", xd, yd, d.x, d.y, search->dx, search->dy);
			}*/

			// ignore opposed arrows
			if (search->inner != inner) {
				continue;
			}

			nextArrow = &grid[yn][xn];

			// follow adjacent arrow
			if (nextArrow->type == search->arrow && !nextArrow->seen) {
				xd = xn;
				yd = yn;
				type = search->arrow;
				break;
			}
		}

		// arrow state
		arrow = &states[type][inner];
		// real coordinates
		xr = xd - 1 + arrow->rxd;
		yr = (yd - 1) / 2 + arrow->ryd;
	}
	while (type);
}

/**
 * Output paths.
 *
 * @param x First path arrow.
 * @param y First path arrow.
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param a First arrow.
 * @param grid Grid to search for paths.
 */
static void makePath(int x, int y, int width, int height, struct arrow a, struct arrow grid[height * 2 + 3][width + 3]) {
	int type = a.type;
	int inner = a.inner;
	// current arrow position
	int xd = x;
	int yd = y;

	// arrow state
	struct arrowState const* arrow = &states[type][inner];
	// real coordinates
	int xr = xd - 1 + arrow->rxd;
	int yr = (yd - 1) / 2 + arrow->ryd;
	// previous arrow
	int ap = type;

	// previous path coordinates
	int xp = xr;
	int yp = yr;

	printf("M %d %d", xr, yr);

	do {
		struct arrow* currentArrow = &grid[yd][xd];

		type = ARROW_NONE;
		// mark as followed
		currentArrow->followed = 1;

		// search for adjacent arrows in precedence order
		for (int n = 0; n < 4; n++) {
			// adjacent arrow
			struct arrowNext const* search = &arrow->next[n];
			int xn = xd + search->dx;
			int yn = yd + search->dy;
			struct arrow const* nextArrow = &grid[yn][xn];

			// follow adjacent arrow
			if (nextArrow->type == search->arrow && !nextArrow->followed) {
				if (search->inner != inner) {
					if (nextArrow->inner == currentArrow->inner) {
						continue;
					}
					else {
						continue;
						//
						// TODO:
						// - Follow arrows
						// - If opposed by arrow of different color
						//    - If red, follow right path
						//    - If green, follow left path
						//
					}
				}

				xd = xn;
				yd = yn;
				type = search->arrow;
				break;
			}
		}

		// arrow state
		arrow = &states[type][inner];
		// real coordinates
		xr = xd - 1 + arrow->rxd;
		yr = (yd - 1) / 2 + arrow->ryd;

		// end path segment if arrow changes
		// and ignore last path segment
		if (type != ap && type) {
			switch (ap) {
				// arrow of current path segment
				case ARROW_RIGHT:
				case ARROW_LEFT: {
					printf("h%d", xr - xp);
					break;
				}
				case ARROW_DOWN:
				case ARROW_UP: {
					printf("v%d", yr - yp);
					break;
				}
			}

			xp = xr;
			yp = yr;
			ap = type;
		}
	}
	while (type);

	printf("z");
}

/**
 * Search all paths in arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Grid to search for paths.
 */
static void searchPaths(int width, int height, struct arrow grid[height * 2 + 3][width + 3]) {
	int awidth = width + 3;
	int aheight = height * 2 + 3;

	// search right and left arrows in grid
	for (int y = 1; y < aheight - 1; y += 2) {
		for (int x = 1; x < awidth - 1; x++) {
			struct arrow a = grid[y][x];

			if (a.type && !a.seen) {
				preparePath(x, y, width, height, a, grid);
			}
		}
	}

	// search right and left arrows in grid
	for (int y = 1; y < aheight - 1; y += 2) {
		for (int x = 1; x < awidth - 1; x++) {
			struct arrow a = grid[y][x];

			if (a.type && !a.followed) {
				makePath(x, y, width, height, a, grid);
			}
		}
	}
}

/**
 * Fill arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param data The bitmap.
 * @param grid Grid to fill with arrows.
 */
static void setArrows(int width, int height, char const map[height][width], struct arrow grid[height * 2 + 3][width + 3]) {
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
 * Create arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @return Arrow grid.
 */
static struct arrow* createArrowGrid(int width, int height) {
	return calloc((width + 1 + 3) * (height * 2 + 3), sizeof(struct arrow));
}

/**
 * Follow path at given position.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param map The bitmap to print.
 * @param grid The arrow grid to print.
 */
static void printGrid(int width, int height, char const map[height][width], struct arrow grid[height * 2 + 3][width + 3]) {
	static const char* arrows[] = {
		[ARROW_NONE]  = "∙",
		[ARROW_RIGHT] = "→",
		[ARROW_LEFT]  = "←",
		[ARROW_DOWN]  = "↓",
		[ARROW_UP]    = "↑",
	};

	int ewidth = width + 3;
	int eheight = height * 2 + 3;

	for (int y = 0; y < eheight; y++) {
		if (y % 2 != 0) {
			printf("  ");
		}

		for (int x = 0; x < ewidth - (y % 2 != 0); x++) {
			struct arrow a = grid[y][x];
			int type = a.type;
			char const* color = "";

			if (type) {
				color = colors[a.inner ? COLOR_RED : COLOR_GREEN];
			}

			printf("%s%s%s", color, arrows[(int)type], colors[COLOR_RESET]);

			if (x > 0 && y >= 2 && x < ewidth - 2 && y < eheight - 2 && (y % 2) == 0) {
				printf(" %c ", map[(y - 2) / 2][x - 1] ? '#' : ' ');
			}
			else {
				printf("   ");
			}
		}

		printf("\n");
	}
}

int getDir(int a, int b) {
	struct delta d = directions[a][b];

	return d.x + d.y;
}

int main() {
	const int width = WIDTH;
	const int height = HEIGHT;

	struct arrow* grid = createArrowGrid(width, height);

	setArrows(width, height, (void const*)map, (void*)grid);

	printf("<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"", width, height);
	searchPaths(width, height, (void*)grid);
	printf("\"></path></svg>\n");

	printf("\n");
	printGrid(width, height, (void const*)map, (void*)grid);
	printf("\n");

	return 0;
}
