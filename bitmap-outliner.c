#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bitmap-outliner.h"

#define MIN_SEGMENTS_COUNT 64

/**
 * Information about how to proceed to next arrow.
 */
typedef struct {
	arrow_type arrow; ///< Type of arrow.
	int8_t dx;        ///< Relative to current position.
	int8_t dy;        ///< Relative to current position.
} arrow_next;

/**
 * The arrow states.
 */
static arrow_next const states[][2][3] = {
	[ARROW_RIGHT] = {
		{{ARROW_UP,   +1, -1}, {ARROW_RIGHT, +1,  0}, {ARROW_DOWN, +1, +1}},
		{{ARROW_DOWN, +1, +1}, {ARROW_RIGHT, +1,  0}, {ARROW_UP,   +1, -1}},
	},
	[ARROW_LEFT] = {
		{{ARROW_DOWN,  0, +1}, {ARROW_LEFT,  -1,  0}, {ARROW_UP,    0, -1}},
		{{ARROW_UP,    0, -1}, {ARROW_LEFT,  -1,  0}, {ARROW_DOWN,  0, +1}},
	},
	[ARROW_DOWN] = {
		{{ARROW_RIGHT, 0, +1}, {ARROW_DOWN,   0, +2}, {ARROW_LEFT, -1, +1}},
		{{ARROW_LEFT, -1, +1}, {ARROW_DOWN,   0, +2}, {ARROW_RIGHT, 0, +1}},
	},
	[ARROW_UP] = {
		{{ARROW_LEFT, -1, -1}, {ARROW_UP,     0, -2}, {ARROW_RIGHT, 0, -1}},
		{{ARROW_RIGHT, 0, -1}, {ARROW_UP,     0, -2}, {ARROW_LEFT, -1, -1}},
	},
};

/**
 * Fill arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param map The bitmap.
 * @param grid Grid to fill with arrows.
 */
static void setArrowsFromMap(int width, int height, uint8_t const map[height][width], grid_arrow grid[height * 2 + 3][width + 3]) {
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
 * Convert arrow grid coordinates to path coordinates.
 *
 * @param type Arrow type.
 * @param gx Grid X-coordinate.
 * @param gy Grid Y-coordinate.
 * @param rx Path X-coordinates.
 * @param ry Path Y-coordinates.
 */
static void realCoords(arrow_type type, int32_t gx, int32_t gy, int32_t* rx, int32_t* ry) {
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
		[ARROW_RIGHT] = {0, 0},
		[ARROW_LEFT]  = {1, 0},
		[ARROW_DOWN]  = {0, 0},
		[ARROW_UP]    = {0, 1},
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
static int outliner_grow_segments(outliner* outliner) {
	path_segment* segments = outliner->segments;
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
 * Mark arrow as outer and inner.
 *
 * @param x First path arrow.
 * @param y First path arrow.
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param a First arrow.
 * @param grid Grid to search for paths.
 */
static int makePath(outliner* outliner, int x, int y, int width, int height, grid_arrow grid[height * 2 + 3][width + 3]) {
	int xd = x;
	int yd = y;
	int xr, yr;
	int xp, yp;
	grid_arrow* currentArrow = &grid[yd][xd];
	grid_arrow* nextArrow = currentArrow;
	arrow_type type = currentArrow->type;
	int inner = (type == ARROW_LEFT);
	arrow_type prevtype = type;
	int segments_count = outliner->segments_count;
	path_segment* segments = outliner->segments;
	path_segment* segment;

	realCoords(type, xd, yd, &xr, &yr);

	xp = xr;
	yp = yr;

	segment = &segments[segments_count++];
	segment->type = ARROW_NONE;
	segment->dx = xr;
	segment->dy = yr;

	do {
		arrow_next const* arrows = &states[type][inner][0];

		currentArrow = nextArrow;
		type = ARROW_NONE;

		// mark as seen
		currentArrow->seen = 1;
		currentArrow->inner = inner;

		// search for adjacent arrows in precedence order
		for (int n = 0; n < 3; n++) {
			arrow_next const* search = &arrows[n];
			int xn = xd + search->dx;
			int yn = yd + search->dy;

			nextArrow = &grid[yn][xn];

			// follow adjacent arrow
			if (nextArrow->type == search->arrow && !nextArrow->seen) {
				xd = xn;
				yd = yn;
				type = search->arrow;
				break;
			}
		}

		// end path segment if arrow changes
		// and ignore last path segment
		if (type != prevtype && type) {
			int dx, dy;

			realCoords(type, xd, yd, &xr, &yr);

			dx = xr - xp;
			dy = yr - yp;

			if (segments_count >= outliner->segments_cap) {
				if (outliner_grow_segments(outliner) != 0) {
					return 0;
				}

				segments = outliner->segments;
			}

			segment = &segments[segments_count++];
			segment->type = prevtype;
			segment->dx = dx;
			segment->dy = dy;

			xp = xr;
			yp = yr;
			prevtype = type;
		}
	}
	while (type);

	outliner->segments_count = segments_count;

	return segments_count;
}

/**
 * Search all paths in arrow grid.
 *
 * @param width Width of bitmap.
 * @param height Height of bitmap.
 * @param grid Grid to search for paths.
 */
static void searchPaths(outliner* outliner, int width, int height, grid_arrow grid[height * 2 + 3][width + 3]) {
	int gridWidth = width + 3;
	int gridHeight = height * 2 + 3;

	// search right and left arrows in grid
	for (int y = 1; y < gridHeight - 1; y += 2) {
		for (int x = 1; x < gridWidth - 1; x++) {
			grid_arrow arrow = grid[y][x];

			if (arrow.type && !arrow.seen) {
				makePath(outliner, x, y, width, height, grid);
			}
		}
	}
}

int outliner_init(outliner* object, int width, int height, uint8_t const* data) {
	*object = (outliner) {
		.width = width,
		.height = height,
		.data = data,
	};

	object->arrow_grid = calloc((width + 1 + 3) * (height * 2 + 3), sizeof(*object->arrow_grid));

	if (!object->arrow_grid) {
		goto error;
	}


	if (outliner_grow_segments(object) != 0) {
		goto error;
	}

	return 0;

	error: {
		free(object);

		return -1;
	}
}

void outliner_free(outliner* outliner) {
	free(outliner->arrow_grid);
	free(outliner->segments);
}

path_segment const* outliner_find_paths(outliner* outliner, int* out_length) {
	int width = outliner->width;
	int height = outliner->height;
	grid_arrow* grid = outliner->arrow_grid;
	uint8_t const* data = outliner->data;

	outliner->segments_count = 0;

	setArrowsFromMap(width, height, (void const*)data, (void*)grid);
	searchPaths(outliner, width, height, (void*)grid);

	*out_length = outliner->segments_count;

	return outliner->segments;
}
