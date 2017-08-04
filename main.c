#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 bitmap-outliner.c bitmap-outliner-print.c -lqrencode \
	&& /tmp/`basename $0` $@
exit
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "qrencode.h"
#include "bitmap-outliner.h"
#include "bitmap-outliner-print.h"

#define WIDTH 7
#define HEIGHT 7


uint8_t const map[] = {
	1, 1, 0, 1 ,0, 1, 1,
	0, 0, 1, 0 ,1, 0, 0,
	1, 0, 1, 1 ,1, 0, 1,
	1, 1, 0, 0 ,0, 1, 1,
	1, 1, 0, 0 ,0, 1, 1,
	1, 1, 0, 1 ,0, 1, 1,
	1, 1, 0, 1 ,0, 1, 1,
};


/*
uint8_t const map[] = {
	0, 1, 1, 1, 1,
	1, 0, 1, 1, 1,
	1, 1, 0, 1, 1,
	1, 1, 1, 1, 1,
	1, 1, 1, 1, 1,
};
*/

/*
char const map[] = {
	0, 1, 0,
	1, 0, 1,
	0, 1, 0,
};
*/

static void print_svg(int width, int height, bmol_path_seg const* segments, int count) {
	printf("<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"", width, height);

	for (int i = 0; i < count; i++) {
		bmol_path_seg const* segment = &segments[i];

		switch (segment->type) {
			case BMOL_ARR_NONE: {
				if (i > 0) {
					printf("z");
				}

				printf("M %d %d", segment->dx, segment->dy);
				break;
			}
			case BMOL_ARR_RIGHT:
			case BMOL_ARR_LEFT: {
				printf("h%d", segment->dx);
				break;
			}
			case BMOL_ARR_DOWN:
			case BMOL_ARR_UP: {
				printf("v%d", segment->dy);
				break;
			}
			default: {
				break;
			}
		}
	}

	printf("z");

	printf("\"></path></svg>\n");
}

int main() {
	int width = WIDTH;
	int height = HEIGHT;
	uint8_t* data = (uint8_t*)map;

	bmol_outliner outliner;

	QRcode* code = QRcode_encodeString("https://monoxid.net", 0, 0, QR_MODE_8, 0);

	width = code->width;
	height = code->width;

	//printf("width: %d\nheight: %d\n", width, height);

	for (int i = 0; i < width * height; i++) {
		code->data[i] &= 1;

		/*if (i && i % width == 0) {
			printf("\n");
		}

		printf("%d, ", code->data[i]);*/
	}

	//printf("\n\n");

	data = (uint8_t*)code->data;

	bmol_init(&outliner, width, height, data);

	int count;
	bmol_path_seg const* segments = bmol_outliner_find_paths(&outliner, &count);

	if (!segments) {
		fprintf(stderr, "Alloction error\n");
		exit(1);
	}

	print_svg(width, height, segments, count);

	printf("\n");
	bmol_print_grid(&outliner);
	printf("\n");

	bmol_free(&outliner);
	QRcode_free(code);

	return 0;
}
