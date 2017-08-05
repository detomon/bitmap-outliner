#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bitmap-outliner.h"

#define MIN_SEGMENTS_COUNT 64

/**
 * Information about how to proceed to next arrow.
 */
typedef struct {
	bmol_arr_type arrow; ///< Type of arrow.
	int8_t dx;           ///< Relative to current position.
	int8_t dy;           ///< Relative to current position.
} const arrow_next;

/**
 * The arrow states.
 */
static arrow_next const states[][2][4] = {
	[BMOL_ARR_RIGHT] = {
		{
			{BMOL_ARR_LEFT,  +1,  0}, {BMOL_ARR_UP,   +1, -1},
			{BMOL_ARR_RIGHT, +1,  0}, {BMOL_ARR_DOWN, +1, +1},
		}, {
			{BMOL_ARR_DOWN,  +1, +1}, {BMOL_ARR_DOWN, +1, +1},
			{BMOL_ARR_RIGHT, +1,  0}, {BMOL_ARR_UP,   +1, -1},
		},
	},
	[BMOL_ARR_LEFT] = {
		{
			{BMOL_ARR_RIGHT, -1,  0}, {BMOL_ARR_DOWN,  0, +1},
			{BMOL_ARR_LEFT,  -1,  0}, {BMOL_ARR_UP,    0, -1},
		}, {
			{BMOL_ARR_UP,     0, -1}, {BMOL_ARR_UP,    0, -1},
			{BMOL_ARR_LEFT,  -1,  0}, {BMOL_ARR_DOWN,  0, +1},
		},
	},
	[BMOL_ARR_DOWN] = {
		{
			{BMOL_ARR_UP,     0, +2}, {BMOL_ARR_RIGHT, 0, +1},
			{BMOL_ARR_DOWN,   0, +2}, {BMOL_ARR_LEFT, -1, +1}
		}, {
			{BMOL_ARR_LEFT,  -1, +1}, {BMOL_ARR_LEFT, -1, +1},
			{BMOL_ARR_DOWN,   0, +2}, {BMOL_ARR_RIGHT, 0, +1},
		},
	},
	[BMOL_ARR_UP] = {
		{
			{BMOL_ARR_DOWN,   0, -2}, {BMOL_ARR_LEFT, -1, -1},
			{BMOL_ARR_UP,     0, -2}, {BMOL_ARR_RIGHT, 0, -1},
		}, {
			{BMOL_ARR_RIGHT,  0, -1}, {BMOL_ARR_RIGHT, 0, -1},
			{BMOL_ARR_UP,     0, -2}, {BMOL_ARR_LEFT, -1, -1},
		},
	},
};

/**
 * Convert arrow grid coordinates to path coordinates.
 *
 * @param type Arrow type.
 * @param gx Grid X-coordinate.
 * @param gy Grid Y-coordinate.
 * @param rx Path X-coordinates.
 * @param ry Path Y-coordinates.
 */
static void real_coords(bmol_arr_type type, int gx, int gy, int* rx, int* ry) {
	/**
	 * Defines coordinates.
	 */
	typedef struct {
		int x; ///< X-coordinate.
		int y; ///< Y-coordinate.
	} coords;

	/**
	 * Delta to add to convert to real coordinates.
	 */
	static coords const real_delta[] = {
		[BMOL_ARR_RIGHT] = {0, 0},
		[BMOL_ARR_LEFT]  = {1, 0},
		[BMOL_ARR_DOWN]  = {0, 0},
		[BMOL_ARR_UP]    = {0, 1},
	};

	coords const* real = &real_delta[type];

	*rx = gx - 1 + real->x;
	*ry = (gy - 1) / 2 + real->y;
}

/**
 * Resize segments count.
 *
 * @param outliner The outline object.
 * @return 0 on success.
 */
static int bmol_outliner_grow_segments(bmol_outliner* outliner) {
	bmol_path_seg* segments = outliner->segments;
	int segments_cap = outliner->segments_cap * 2 + 1;

	if (segments_cap < MIN_SEGMENTS_COUNT) {
		segments_cap = MIN_SEGMENTS_COUNT;
	}

	segments = realloc(segments, sizeof(*segments) * segments_cap);

	if (!segments) {
		return -1;
	}

	outliner->segments = segments;
	outliner->segments_cap = segments_cap;

	return 0;
}

/**
 * Add segment to segment list.
 *
 * @param outliner The outline object.
 * @param type The arrow type.
 * @param dx Path segment X-coordinate.
 * @param dy Path segment Y-coordinate.
 * @return 0 on success.
 */
static int push_segment(bmol_outliner* outliner, bmol_arr_type type, int dx, int dy) {
	bmol_path_seg* segment;
	bmol_path_seg* segments = outliner->segments;

	if (outliner->segments_count >= outliner->segments_cap) {
		if (bmol_outliner_grow_segments(outliner) < 0) {
			return -1;
		}

		segments = outliner->segments;
	}

	segment = &segments[outliner->segments_count++];
	segment->type = type;
	segment->dx = dx;
	segment->dy = dy;

	return 0;
}

/**
 * Search adjacent arrow.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Arrow grid.
 * @param type Current arrow type.
 * @param inner Is inner path.
 * @param xd Arrow X-coordinate.
 * @param yd Arrow Y-coordinate.
 */
static bmol_arrow* search_adjacent_arrow(int width, int height, bmol_arrow grid[height * 2 + 3][width + 3], bmol_arr_type type, int inner, int* xd, int* yd) {
	arrow_next* arrows = &states[type][inner][0];

	// search for adjacent arrows in precedence order
	for (int n = 0; n < 4; n++) {
		arrow_next* search = &arrows[n];
		int xn = *xd + search->dx;
		int yn = *yd + search->dy;
		bmol_arrow* nextArrow = &grid[yn][xn];

		// follow adjacent arrow
		if (nextArrow->type == search->arrow && !nextArrow->seen) {
			// is opposite arrow
			if (n == 0) {
				if (!inner && nextArrow->inner) {
					*xd = xn;
					*yd = yn;

					nextArrow->seen = 0; // do not mark opposite arrow as seen
					type = nextArrow->type;

					// search next inner arrow relative to opposite arrow
					return search_adjacent_arrow(width, height, grid, type, 1, xd, yd);
				}
				else {
					continue;
				}
			}
			// ignore arrows not in path type
			else if (nextArrow->inner != inner) {
				continue;
			}

			*xd = xn;
			*yd = yn;

			return nextArrow;
		}
	}

	// switch to outer path if no more inner path arrows found
	if (inner) {
		return search_adjacent_arrow(width, height, grid, type, 0, xd, yd);
	}

	return NULL;
}

/**
 * Make path segments.
 *
 * @param x First path arrow.
 * @param y First path arrow.
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Grid to search for paths.
 */
static int make_path(bmol_outliner* outliner, int x, int y, int width, int height, bmol_arrow grid[height * 2 + 3][width + 3]) {
	int xd = x;
	int yd = y;
	int xr, yr;
	int xp, yp;
	bmol_arrow* arrow = &grid[yd][xd];
	bmol_arrow* nextArrow = arrow;
	bmol_arr_type type = arrow->type;
	int inner = (type == BMOL_ARR_LEFT);
	bmol_arr_type prevType = type;

	real_coords(type, xd, yd, &xr, &yr);

	xp = xr;
	yp = yr;

	// begin path
	if (push_segment(outliner, BMOL_ARR_NONE, xr, yr) != 0) {
		return -1;
	}

	do {
		arrow = nextArrow;
		arrow->seen = 1; // mark as seen

		nextArrow = search_adjacent_arrow(width, height, grid, type, inner, &xd, &yd);
		type = BMOL_ARR_NONE;

		if (nextArrow) {
			type = nextArrow->type;
			inner = nextArrow->inner; // switch arrow type
		}

		// end path segment if arrow changes
		// and ignore last path segment
		if (type != prevType && type) {
			int dx, dy;

			real_coords(type, xd, yd, &xr, &yr);

			dx = xr - xp;
			dy = yr - yp;
			xp = xr;
			yp = yr;

			// add path segment
			if (push_segment(outliner, prevType, dx, dy) < 0) {
				return-1;
			}

			prevType = type;
		}
	}
	while (type);

	return 0;
}

/**
 * Mark arrow as outer and inner.
 *
 * @param x First path arrow.
 * @param y First path arrow.
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Grid to search for paths.
 */
static void set_path_type(bmol_outliner* outliner, int x, int y, int width, int height, bmol_arrow grid[height * 2 + 3][width + 3]) {
	bmol_arrow* arrow = &grid[y][x];
	bmol_arr_type type = arrow->type;
	int inner = (type == BMOL_ARR_LEFT);

	do {
		arrow_next* arrows = &states[type][inner][1]; // ignore opponent arrow

		// mark as visited
		arrow->visited = 1;
		arrow->inner = inner;

		type = BMOL_ARR_NONE;

		// search for adjacent arrows in precedence order
		for (int n = 0; n < 3; n++) {
			arrow_next* search = &arrows[n];
			int xn = x + search->dx;
			int yn = y + search->dy;
			bmol_arrow* nextArrow = &grid[yn][xn];

			// follow adjacent arrow
			if (nextArrow->type == search->arrow && !nextArrow->visited) {
				x = xn;
				y = yn;
				type = nextArrow->type;
				arrow = nextArrow;
				break;
			}
		}
	}
	while (type);
}

/**
 * Search all paths in arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Grid to search for paths.
 */
static int search_paths(bmol_outliner* outliner, int width, int height, bmol_arrow grid[height * 2 + 3][width + 3]) {
	int gridWidth = width + 3;
	int gridHeight = height * 2 + 3;

	// set arrow types
	for (int y = 1; y < gridHeight - 1; y += 2) {
		for (int x = 1; x < gridWidth - 1; x++) {
			bmol_arrow arrow = grid[y][x];

			if (arrow.type && !arrow.visited) {
				set_path_type(outliner, x, y, width, height, grid);
			}
		}
	}

	// search right and left arrows in grid
	for (int y = 1; y < gridHeight - 1; y += 2) {
		for (int x = 1; x < gridWidth - 1; x++) {
			bmol_arrow arrow = grid[y][x];

			if (arrow.type && !arrow.seen) {
				if (make_path(outliner, x, y, width, height, grid) < 0) {
					return -1;
				}
			}
		}
	}

	return 0;
}

/**
 * Fill arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param map The bitmap.
 * @param grid Grid to fill with arrows.
 */
static void set_arrows(int width, int height, uint8_t const map[height][width], bmol_arrow grid[height * 2 + 3][width + 3]) {
	int x, y, p, t;

	for (x = 0; x < width; x++) {
		for (y = 0, t = 0; y < height; y++) {
			if ((p = map[y][x]) != t) {
				grid[y * 2 + 1][x + 1].type = t ? BMOL_ARR_LEFT : BMOL_ARR_RIGHT;
				t = p;
			}
		}

		if (map[y - 1][x]) {
			grid[y * 2 + 1][x + 1].type = BMOL_ARR_LEFT;
		}
	}

	for (y = 0; y < height; y++) {
		for (x = 0, t = 0; x < width; x++) {
			if ((p = map[y][x]) != t) {
				grid[y * 2 + 2][x + 1].type = t ? BMOL_ARR_DOWN : BMOL_ARR_UP;
				t = p;
			}
		}

		if (map[y][x - 1]) {
			grid[y * 2 + 2][x + 1].type = BMOL_ARR_DOWN;
		}
	}
}

bmol_outliner* bmol_alloc(uint8_t const* data, int width, int height) {
	size_t size;
	bmol_outliner* outliner;

	size = (width + 3) * (height * 2 + 3) * sizeof(outliner->arrow_grid[0]);
	outliner = calloc(1, sizeof(*outliner) + size);

	if (!outliner) {
		return NULL;
	}

	outliner->width = width;
	outliner->height = height;
	outliner->data = data;

	if (bmol_outliner_grow_segments(outliner) != 0) {
		free(outliner);

		return NULL;
	}

	return outliner;
}

void bmol_free(bmol_outliner* outliner) {
	free(outliner->segments);
	free(outliner);
}

bmol_path_seg const* bmol_outliner_find_paths(bmol_outliner* outliner, int* out_length) {
	int width = outliner->width;
	int height = outliner->height;
	bmol_arrow* grid = outliner->arrow_grid;
	uint8_t const* data = outliner->data;

	outliner->segments_count = 0;

	set_arrows(width, height, (const uint8_t (*)[width])data, (bmol_arrow (*)[width])grid);

	if (search_paths(outliner, width, height, (bmol_arrow (*)[width])grid) < 0) {
		return NULL;
	}

	*out_length = outliner->segments_count;

	return outliner->segments;
}
