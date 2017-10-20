#if 0
gcc -Wall -O2 -I. -o /tmp/`basename $0` $0 bitmap-outliner.c \
	&& /tmp/`basename $0` $@
exit
#endif

#include <stdio.h>
#include "bitmap-outliner.h"

uint8_t const data[] = {
	0, 1, 1, 1 ,0, 0,
	1, 0, 1, 0 ,0, 1,
	1, 1, 0, 0 ,1, 1,
	1, 0, 0, 1 ,0, 1,
	0, 0, 1, 0 ,1, 1,
	1, 0, 1, 1 ,1, 0,
};

int main() {
	int const width = 6;
	int const height = 6;

	bmol_outliner* outliner = bmol_alloc(width, height, data);

	bmol_find_paths(outliner, NULL);

	size_t path_len = bmol_svg_path_len(outliner);
	char path[path_len];

	bmol_svg_path(outliner, path, path_len);

	printf(
		"<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\">\n"
		"	<path d=\"%s\" fill=\"#000\" fill-rule=\"evenodd\"/>\n"
		"</svg>\n", width, height, path);

	bmol_free(outliner);

	return 0;
}
