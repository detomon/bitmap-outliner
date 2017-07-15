// clang -O2 -Wall -o pixel-map pixel-map.c

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

static void printEdges(int width, int height, char edges[height * 2 + 3][width + 3], char const data[height][width]) {
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
			printf("%s", arrows[(int)edges[y][x]]);

			if (x > 0 && y >= 2 && x < ewidth - 2 && y < eheight - 2 && (y % 2) == 0) {
				printf(" %c ", data[(y - 2) / 2][x - 1] ? '#' : ' ');
			}
			else {
				printf("   ");
			}
		}

		printf("\n");
	}
}

struct arrowState {
	char rxd;
	char ryd;
	struct arrowNext {
		char dx;
		char dy;
		char arrow;
	} next[3];
};

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

static void searchPath(int x, int y, int width, int height, int a, char edges[height * 2 + 3][width + 3]) {
	// assume path is inner path
	int reversed = (a == ARROW_LEFT);
	// set arrow precedence order
	int n0 = reversed ?  2 : 0;
	int ni = reversed ? -1 : 1;
	int nn = reversed ? -1 : 3;
	// current arrow position
	int xd = x;
	int yd = y;

	// arrow state
	struct arrowState const* arrow = &states[a];
	// real coordinates
	int xr = xd - 1 + arrow->rxd;
	int yr = (yd - 1) / 2 + arrow->ryd;
	// previous arrow
	int ap = a;

	// previous path coordinates
	int xp = xr;
	int yp = yr;

	printf("M %d %d", xr, yr);

	do {
		a = ARROW_NONE;
		// clear arrow in grid
		edges[yd][xd] = ARROW_NONE;

		// search for adjacent arrows in precedence order
		for (int n = n0; n != nn; n += ni) {
			// adjacent arrow
			struct arrowNext const* next = &arrow->next[n];
			int xn = xd + next->dx;
			int yn = yd + next->dy;

			// follow adjacent arrow
			if (edges[yn][xn] == next->arrow) {
				xd = xn;
				yd = yn;
				a = next->arrow;
				break;
			}
		}

		// arrow state
		arrow = &states[a];
		// real coordinates
		xr = xd - 1 + arrow->rxd;
		yr = (yd - 1) / 2 + arrow->ryd;

		// end path segment if arrow changes
		// and ignore last path segment
		if (a != ap && a) {
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
			ap = a;
		}
	}
	while (a);

	printf("z");
}

static void searchPaths(int width, int height, char edges[height * 2 + 3][width + 3]) {
	int awidth = width + 3;
	int aheight = height * 2 + 3;

	// search right and left arrows in grid
	for (int y = 1; y < aheight - 1; y += 2) {
		for (int x = 1; x < awidth - 1; x++) {
			int a = edges[y][x];

			if (a) {
				searchPath(x, y, width, height, a, edges);
			}
		}
	}
}

static void setArrows(int width, int height, char edges[height * 2 + 3][width + 3], char const data[height][width]) {
	int x, y, p, t;

	for (x = 0; x < width; x++) {
		for (y = 0, t = 0; y < height; y++) {
			if ((p = data[y][x]) != t) {
				edges[y * 2 + 1][x + 1] = t ? ARROW_LEFT : ARROW_RIGHT;
				t = p;
			}
		}

		if (data[y - 1][x]) {
			edges[y * 2 + 1][x + 1] = ARROW_LEFT;
		}
	}

	for (y = 0; y < height; y++) {
		for (x = 0, t = 0; x < width; x++) {
			if ((p = data[y][x]) != t) {
				edges[y * 2 + 2][x + 1] = t ? ARROW_DOWN : ARROW_UP;
				t = p;
			}
		}

		if (data[y][x - 1]) {
			edges[y * 2 + 2][x + 1] = ARROW_DOWN;
		}
	}
}

static char* createEdges(int width, int height) {
	return calloc((width + 1 + 3) * (height * 2 + 3), sizeof(char));
}

int main() {
	const int width = WIDTH;
	const int height = HEIGHT;

	size_t size = width * height;
	char* data = malloc(size);

	memset(data, -1, size);

	char* edges = createEdges(width, height);

	setArrows(width, height, (void*)edges, (void const*)map);

	printf("<svg viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\"><path d=\"", width, height);
	searchPaths(width, height, (void*)edges);
	printf("\"></path></svg>\n");

	printf("\n");
	printEdges(width, height, (void*)edges, (void const*)map);
	printf("\n");

	return 0;
}
