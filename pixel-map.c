// clang -O2 -Wall -o pixel-map pixel-map.c

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WIDTH 10
#define HEIGHT 5

enum {
	ARROW_NONE = 0,
	ARROW_RIGHT,
	ARROW_LEFT,
	ARROW_DOWN,
	ARROW_UP
};

char const map[] =
	"\x01\x00\x00\x01\x00\x01\x00\x00\x01\x00"
	"\x00\x01\x01\x00\x01\x00\x01\x01\x00\x01"
	"\x00\x01\x01\x01\x00\x00\x01\x01\x01\x00"
	"\x00\x00\x00\x00\x01\x00\x00\x00\x00\x01"
	"\x01\x01\x00\x00\x01\x01\x01\x00\x00\x01";

static int* createEdges(int width, int height) {
	return calloc((width + 1) * (height * 2 + 1), sizeof(int));
}

static void printEdges(int width, int height, int edges[height * 2 + 1][width + 1], char const data[height][width]) {
	static const char* arrows[] = {
		[ARROW_NONE]  = "∙",
		[ARROW_RIGHT] = "→",
		[ARROW_LEFT]  = "←",
		[ARROW_DOWN]  = "↓",
		[ARROW_UP]    = "↑",
	};

	int ewidth = width + 1;
	int eheight = height * 2 + 1;

	for (int y = 0; y < eheight; y++) {
		if (y % 2 == 0) {
			printf("  ");
		}

		for (int x = 0; x < ewidth/* - (y % 2 == 0*/; x++) {
			printf("%s", arrows[edges[y][x]]);

			if (y > 0 && x < width && y < height * 2 && (y % 2) != 0) {
				printf(" %c ", data[y / 2][x] ? '#' : ' ');
			}
			else {
				printf("   ");
			}
		}

		printf("\n");
	}
}

static void addArrows(int width, int height, int edges[height * 2 + 1][width + 1], char const data[height][width]) {
	for (int x = 0; x < width; x ++) {
		for (int y = 0; y <= height; y++) {
			int p1 = (y > 0) ? data[y - 1][x] : 0;
			int p2 = (y < height) ? data[y][x] : 0;

			if (p1 != p2) {
				edges[y * 2][x] = p1 ? ARROW_LEFT : ARROW_RIGHT;
			}
		}
	}

	for (int x = 0; x <= width; x ++) {
		for (int y = 0; y < height; y++) {
			int p1 = (x > 0) ? data[y][x - 1] : 0;
			int p2 = (x < width) ? data[y][x] : 0;

			if (p1 != p2) {
				edges[y * 2 + 1][x] = p1 ? ARROW_DOWN : ARROW_UP;
			}
		}
	}
}

static void makePaths(int width, int height, int edges[height * 2 + 1][width + 1]) {
	int awidth = width + 1;
	int aheight = height * 2 + 1;

	// round y down to multiple of 2 and goto next horizontal arrow row
	for (int y = 0; y < aheight; y = (y & ~1) + 2) {
		for (int x = 0; x < awidth; x++) {
			int a = edges[y][x];
			int xd = x;
			int yd = y;

			if (a) {
				int xr = 0;
				int yr = 0;
				int ra = ARROW_NONE;

				/*
				static int states[5][3][3] = {
					[ARROW_RIGHT] = {
						{+1, -1, ARROW_UP},
						{+1, +0, ARROW_RIGHT},
						{+1, +1, ARROW_DOWN},
					},
					[ARROW_LEFT] = {
						{+0, +1, ARROW_DOWN},
						{-1, +0, ARROW_LEFT},
						{+0, -1, ARROW_UP},
					},
					[ARROW_DOWN] = {
						{+1, +0, ARROW_RIGHT},
						{+2, +0, ARROW_DOWN},
						{+1, -1, ARROW_LEFT},
					},
					[ARROW_UP] = {
						{-1, -1, ARROW_LEFT},
						{-2, +0, ARROW_UP},
						{-1, +0, ARROW_RIGHT},
					},
				};
				*/

				do {
					ra = a;
					a = ARROW_NONE;
					edges[yd][xd] = ARROW_NONE;

					// left and right arrows
					switch (ra) {
						case ARROW_RIGHT: {
							xr = xd;
							yr = yd / 2;

							if (xd < awidth - 1) {
								// has arrow right above
								if (yd > 0 && edges[yd - 1][xd + 1] == ARROW_UP) {
									xd++; yd--;
									a = ARROW_UP;
								}
								// has arrow right
								else if (edges[yd][xd + 1] == ARROW_RIGHT) {
									xd++;
									a = ARROW_RIGHT;
								}
								// has arrow right below
								else if (yd < aheight - 1 && edges[yd + 1][xd + 1] == ARROW_DOWN) {
									xd++; yd++;
									a = ARROW_DOWN;
								}
							}

							break;
						}
						case ARROW_LEFT: {
							xr = xd + 1;
							yr = yd / 2;

							// has arrow left below
							if (yd < aheight - 1 && edges[yd + 1][xd] == ARROW_DOWN) {
								yd++;
								a = ARROW_DOWN;
							}
							// has arrow left
							else if (xd > 0 && edges[yd][xd - 1] == ARROW_LEFT) {
								xd--;
								a = ARROW_LEFT;
							}
							// has arrow left above
							else if (yd > 0 && edges[yd - 1][xd] == ARROW_UP) {
								yd--;
								a = ARROW_UP;
							}

							break;
						}
						case ARROW_DOWN: {
							xr = xd;
							yr = yd / 2;

							if (yd < aheight - 1) {
								// has arrow right below
								if (edges[yd + 1][xd] == ARROW_RIGHT) {
									yd++;
									a = ARROW_RIGHT;
								}
								// has arrow below
								else if (yd < aheight - 2 && edges[yd + 2][xd] == ARROW_DOWN) {
									yd += 2;
									a = ARROW_DOWN;
								}
								// has arrow left down
								else if (xd > 0 && edges[yd + 1][xd - 1] == ARROW_LEFT) {
									xd--; yd++;
									a = ARROW_LEFT;
								}
							}

							break;
						}
						case ARROW_UP: {
							xr = xd;
							yr = yd / 2 + 1;

							if (yd > 0) {
								// has arrow left above
								if (xd > 0 && edges[yd - 1][xd - 1] == ARROW_LEFT) {
									xd--; yd--;
									a = ARROW_LEFT;
								}
								// has arrow above
								else if (yd > 1 && edges[yd - 2][xd] == ARROW_UP) {
									yd -= 2;
									a = ARROW_UP;
								}
								// has arrow right above
								else if (edges[yd - 1][xd] == ARROW_RIGHT) {
									yd--;
									a = ARROW_RIGHT;
								}
							}

							break;
						}
						default: {
							a = ARROW_NONE;
							break;
						}
					}

					printf("m %d %d ", xr, yr);

					//usleep(250 * 1000);
					//printf("\n");
					//printEdges(width, height, (void*)edges, (void const*)map);
				}
				while (a);

				printf("\n");
				printEdges(width, height, (void*)edges, (void const*)map);
				//exit(1);
			}
		}
	}
}

int main() {
	const int width = WIDTH;
	const int height = HEIGHT;

	size_t size = width * height;
	char* data = malloc(size);

	memset(data, -1, size);

	int* edges = createEdges(width, height);

	addArrows(width, height, (void*)edges, (void const*)map);

	makePaths(width, height, (void*)edges);

	printf("\n");
	printEdges(width, height, (void*)edges, (void const*)map);
	printf("\n");

	return 0;
}
