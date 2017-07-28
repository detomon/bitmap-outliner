#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 bitmap-outliner.c -lqrencode && /tmp/`basename $0` $@; exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <qrencode.h>
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


uint8_t const map[] =
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
	uint8_t* data = (uint8_t*)map;

	outliner outliner;

	QRcode* code = QRcode_encodeString("https://monoxid.net", 0, 0, QR_MODE_8, 0);

	width = code->width;
	height = code->width;

	for (int i = 0; i < width * height; i++) {
		code->data[i] &= 1;
	}

	data = (uint8_t*)code->data;

	outliner_init(&outliner, width, height, data);

	/*grid_arrow* grid = outliner.arrow_grid;

	setArrowsFromMap(width, height, (void const*)data, (void*)grid);
	searchPaths(&outliner, width, height, (void*)grid);

	path_segment* segments = outliner.segments;
	int count = outliner.segments_count;*/

	int count;
	path_segment const* segments = outliner_find_paths(&outliner, &count);

	printf("<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"", width, height);

	for (int i = 0; i < count; i++) {
		path_segment const* segment = &segments[i];

		switch (segment->type) {
			case ARROW_NONE: {
				if (i > 0) {
					printf("z");
				}

				printf("M %d %d", segment->dx, segment->dy);
				break;
			}
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

	printf("\"></path></svg>\n");



	printf("\n");
	printGrid(width, height, (void const*)data, (void const*)outliner.arrow_grid);
	printf("\n");

	outliner_free(&outliner);
	QRcode_free(code);

	return 0;
}
