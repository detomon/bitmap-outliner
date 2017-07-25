#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 && /tmp/`basename $0` $@; exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 7
#define HEIGHT 7

enum {
	ARROW_NONE = 0,
	ARROW_RIGHT,
	ARROW_LEFT,
	ARROW_DOWN,
	ARROW_UP
};

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

/**
 * Information about how to proceed to next arrow.
 */
struct arrowState {
	int8_t rxd; ///< Delta to add to x to convert to real coordinates
	int8_t ryd; ///< Delta to add to y to convert to real coordinates
	struct arrowNext {
		int8_t dx;     ///< Relative to current position
		int8_t dy;     ///< Relative to current position
		uint8_t arrow; ///< Type of arrow
	} next[3]; ///< Adjacent arrows
};

/**
 * Defines arrow.
 */
struct arrow {
	uint8_t type:3;  ///< Arrow type
	uint8_t inner:1; ///< Associated path is inner path
	uint8_t seen:1;  ///< If already seen
};

/**
 * The arrow states.
 */
static struct arrowState states[] = {
	[ARROW_RIGHT] = {
		.rxd = +0,
		.ryd = +0,
		.next = {
			{+1, -1, ARROW_UP},
			{+1, +0, ARROW_RIGHT},
			{+1, +1, ARROW_DOWN},
		},
	},
	[ARROW_LEFT] = {
		.rxd = +1,
		.ryd = +0,
		.next = {
			{+0, +1, ARROW_DOWN},
			{-1, +0, ARROW_LEFT},
			{+0, -1, ARROW_UP},
		},
	},
	[ARROW_DOWN] = {
		.rxd = +0,
		.ryd = +0,
		.next = {
			{+0, +1, ARROW_RIGHT},
			{+0, +2, ARROW_DOWN},
			{-1, +1, ARROW_LEFT},
		},
	},
	[ARROW_UP] = {
		.rxd = +0,
		.ryd = +1,
		.next = {
			{-1, -1, ARROW_LEFT},
			{+0, -2, ARROW_UP},
			{+0, -1, ARROW_RIGHT},
		},
	},
};

/**
 * Follow path at given position.
 *
 * @param x First path arrow.
 * @param y First path arrow.
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param a First arrow.
 * @param grid Grid to search for paths.
 */
static void searchPath(int x, int y, int width, int height, struct arrow a, struct arrow grid[height * 2 + 3][width + 3]) {
	int type = a.type;
	// assume path is inner path
	int reversed = (type == ARROW_LEFT);
	// set arrow precedence order
	int n0 = reversed ?  2 : 0;
	int ni = reversed ? -1 : 1;
	int nn = reversed ? -1 : 3;
	// current arrow position
	int xd = x;
	int yd = y;

	// arrow state
	struct arrowState const* arrow = &states[type];
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
		type = ARROW_NONE;
		// clear arrow in grid
		grid[yd][xd].type = ARROW_NONE;

		// search for adjacent arrows in precedence order
		for (int n = n0; n != nn; n += ni) {
			// adjacent arrow
			struct arrowNext const* next = &arrow->next[n];
			int xn = xd + next->dx;
			int yn = yd + next->dy;

			// follow adjacent arrow
			if (grid[yn][xn].type == next->arrow) {
				xd = xn;
				yd = yn;
				type = next->arrow;
				break;
			}
		}

		// arrow state
		arrow = &states[type];
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

			if (a.type) {
				searchPath(x, y, width, height, a, grid);
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
			printf("%s", arrows[(int)grid[y][x].type]);

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
