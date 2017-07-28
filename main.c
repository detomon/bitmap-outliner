#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 bitmap-outliner.c bitmap-outliner-print.c -lqrencode && /tmp/`basename $0` $@; exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <qrencode.h>
#include "bitmap-outliner.h"
#include "bitmap-outliner-print.h"

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

static void print_svg(outliner const* outliner, path_segment const* segments, int count) {
	int width = outliner->width;
	int height = outliner->width;

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
}

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

	int count;
	path_segment const* segments = outliner_find_paths(&outliner, &count);

	print_svg(&outliner, segments, count);

	printf("\n");
	outliner_print_grid(&outliner);
	printf("\n");

	outliner_free(&outliner);
	QRcode_free(code);

	return 0;
}
