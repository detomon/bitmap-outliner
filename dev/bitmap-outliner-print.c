#include <stdio.h>
#include "bitmap-outliner.h"

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
 * Print arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param data The bitmap to print.
 * @param grid The arrow grid to print.
 */
static void print_grid(int width, int height, uint8_t const data[height][width], bmol_arrow const grid[height * 2 + 3][width + 3]) {
	static char const* arrows[] = {
		[BMOL_ARR_NONE]  = "∙",
		[BMOL_ARR_RIGHT] = "→",
		[BMOL_ARR_LEFT]  = "←",
		[BMOL_ARR_DOWN]  = "↓",
		[BMOL_ARR_UP]    = "↑",
	};

	int gridWidth = width + 3;
	int gridHeight = height * 2 + 3;

	for (int y = 0; y < gridHeight; y++) {
		if (y % 2 != 0) {
			printf("  ");
		}

		for (int x = 0; x < gridWidth - (y % 2 != 0); x++) {
			bmol_arrow a = grid[y][x];
			int type = a.type;
			char const* color = "";

			if (type) {
				color = colors[a.inner ? COLOR_RED : COLOR_GREEN];
			}

			printf("%s%s%s", color, arrows[(int)type], colors[COLOR_RESET]);

			if (x > 0 && y >= 2 && x < gridWidth - 2 && y < gridHeight - 2 && (y % 2) == 0) {
				printf(" %c ", data[(y - 2) / 2][x - 1] ? '#' : ' ');
			}
			else {
				printf("   ");
			}
		}

		printf("\n");
	}
}

void bmol_print_grid(bmol_outliner const* outliner) {
	int width = outliner->width;
	int height = outliner->height;

	print_grid(width, height, (uint8_t const (*)[width])outliner->data, (bmol_arrow (*)[width])outliner->arrow_grid);
}
