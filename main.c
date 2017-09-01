#if 0
clang -Wall -O2 -o /tmp/`basename $0` $0 -I. bitmap-outliner.c dev/bitmap-outliner-print.c dev/bitmap-outliner-svg.c -lqrencode \
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
#include "dev/bitmap-outliner-svg.h"
#include "dev/bitmap-outliner-print.h"

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
	0, 1, 1, 1 ,0, 0,
	1, 0, 1, 0 ,0, 1,
	1, 1, 0, 0 ,1, 1,
	1, 0, 0, 1 ,0, 1,
	0, 0, 1, 0 ,1, 1,
	1, 0, 1, 1 ,1, 0,
};
*/

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

int main() {
	int width = WIDTH;
	int height = HEIGHT;
	uint8_t* data = (uint8_t*)map;

	QRcode* code = QRcode_encodeString("https://monoxid.net", 0, 0, QR_MODE_8, 0);

	// width = code->width;
	// height = code->width;

	// //printf("width: %d\nheight: %d\n", width, height);

	// for (int i = 0; i < width * height; i++) {
	// 	code->data[i] &= 1;

	// 	/*if (i && i % width == 0) {
	// 		printf("\n");
	// 	}

	// 	printf("%d, ", code->data[i]);*/
	// }

	// //printf("\n\n");

	// data = (uint8_t*)code->data;

	bmol_outliner* outliner = bmol_alloc(width, height);

	bmol_set_bitmap(outliner, data);

	int count;
	bmol_path_seg const* segments = bmol_outliner_find_paths(outliner, &count);

	if (!segments) {
		fprintf(stderr, "Allocation error\n");
		exit(1);
	}

	size_t size;
	char path[1024];

	bmol_outliner_svg_path(outliner, path, sizeof(path), &size);

	printf("<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"%s\" fill=\"#000\" fill-rule=\"evenodd\"/></svg>\n", width, height, path);

	printf("\n");
	bmol_print_grid(outliner);
	printf("\n");

	bmol_free(outliner);
	QRcode_free(code);

	return 0;
}
