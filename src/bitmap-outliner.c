#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bitmap-outliner.h"

#define MIN_SEGMENTS_COUNT 64

/**
 * Information about how to proceed to next arrow.
 */
typedef struct {
	bmol_arr_type arrow:8; ///< Type of arrow.
	int8_t dx;             ///< Relative to current position.
	int8_t dy;             ///< Relative to current position.
} const arrow_next;

/**
 * String buffer context.
 */
typedef struct {
	char* buffer;    ///< A string buffer.
	size_t buf_size; ///< The remaining buffer size
	size_t size;     ///< The buffer size.
} buffer_ctx;

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
		int8_t x; ///< X-coordinate.
		int8_t y; ///< Y-coordinate.
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
 * Allocate more segments.
 *
 * @param outliner The outline object.
 * @return 0 on success.
 */
static int grow_segments(bmol_outliner* outliner) {
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
	bmol_path_seg* segments = outliner->segments;

	if (outliner->segments_size >= outliner->segments_cap) {
		if (grow_segments(outliner) < 0) {
			return -1;
		}

		segments = outliner->segments;
	}

	bmol_path_seg* segment = &segments[outliner->segments_size++];

	segment->type = type;
	segment->dx = dx;
	segment->dy = dy;

	return 0;
}

/**
 * Search adjacent arrow relative to given position.
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
		arrow_next const* search = &arrows[n];
		int const xn = *xd + search->dx;
		int const yn = *yd + search->dy;
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
	int const inner = (type == BMOL_ARR_LEFT);

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
	int const gridWidth = width + 3;
	int const gridHeight = height * 2 + 3;

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
	int x, y, t;

	for (x = 0; x < width; x++) {
		for (y = 0, t = 0; y < height; y++) {
			int p = map[y][x] != 0;

			if (p != t) {
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
			int p = map[y][x] != 0;

			if (p != t) {
				grid[y * 2 + 2][x + 1].type = t ? BMOL_ARR_DOWN : BMOL_ARR_UP;
				t = p;
			}
		}

		if (map[y][x - 1]) {
			grid[y * 2 + 2][x + 1].type = BMOL_ARR_DOWN;
		}
	}
}

bmol_outliner* bmol_alloc(int width, int height, uint8_t const* data) {
	bmol_outliner* outliner;
	size_t const size = (width + 3) * (height * 2 + 3) * sizeof(outliner->arrow_grid[0]);

	outliner = calloc(1, sizeof(*outliner) + size);

	if (!outliner) {
		return NULL;
	}

	outliner->width = width;
	outliner->height = height;
	outliner->data = data;

	if (grow_segments(outliner) != 0) {
		bmol_free(outliner);

		return NULL;
	}

	return outliner;
}

void bmol_free(bmol_outliner* outliner) {
	free(outliner->segments);
	free(outliner);
}

bmol_path_seg const* bmol_find_paths(bmol_outliner* outliner, int* out_size) {
	int const width = outliner->width;
	int const height = outliner->height;
	bmol_arrow* grid = outliner->arrow_grid;
	uint8_t const* data = outliner->data;

	if (!data) {
		return NULL;
	}

	outliner->segments_size = 0;

	set_arrows(width, height, (const uint8_t (*)[width])data, (bmol_arrow (*)[width])grid);

	if (search_paths(outliner, width, height, (bmol_arrow (*)[width])grid) < 0) {
		return NULL;
	}

	if (out_size) {
		*out_size = outliner->segments_size;
	}

	return outliner->segments;
}

/**
 * Append formated string to buffer.
 *
 * @param buffer_ref A reference to the buffer.
 * @param size_ref A reference to the buffer size.
 * @param format The number format.
 */
static void append_string(buffer_ctx* ctx, char const* format, ...) {
	int size;
	va_list args;

	va_start(args, format);
	size = vsnprintf(ctx->buffer, ctx->buf_size, format, args);
	va_end(args);

	if (size > 0) {
		if (size >= ctx->buf_size) {
			size = ctx->buf_size - 1;
		}

		ctx->buf_size -= size;
		ctx->buffer += size;
		ctx->size += size;
	}
}

/**
 * Calculate log2 of an integer.
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
 *
 * @param n The value to get the log2 from.
 * @return log2 of the given integer.
 */
static uint32_t log2_fast(uint32_t n) {
	uint32_t r;
	uint32_t shift;

	r =     (n > 0xFFFF) << 4; n >>= r;
	shift = (n > 0xFF  ) << 3; n >>= shift; r |= shift;
	shift = (n > 0xF   ) << 2; n >>= shift; r |= shift;
	shift = (n > 0x3   ) << 1; n >>= shift; r |= shift;
	                                        r |= (n >> 1);

	return r;
}

/**
 * Calculate log10 of an integer.
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
 *
 * @param n The value to get the log10 from.
 * @return log10 of the given integer.
 */
static int log10_fast(uint32_t n) {
	static uint32_t const pow_10[] = {
		1, 10, 100, 1000, 10000, 100000,
		1000000, 10000000, 100000000, 1000000000
	};

	int t = (log2_fast(n) + 1) * 1233 >> 12;
	int r = t - (n < pow_10[t]);

	return r;
}

/**
 * Write SVG path to string buffer.
 *
 * @param segments The path segments.
 * @param count The number of path segments.
 * @param buffer_ref A reference to the buffer.
 * @param size_ref A reference to the buffer size.
 */
static void write_svg(bmol_path_seg const* segments, int count, buffer_ctx* ctx) {
	for (int i = 0; i < count; i++) {
		bmol_path_seg const* segment = &segments[i];

		switch (segment->type) {
			case BMOL_ARR_NONE: {
				if (i > 0) {
					append_string(ctx, "z");
				}

				append_string(ctx, "M%d,%d", segment->dx, segment->dy);
				break;
			}
			case BMOL_ARR_RIGHT:
			case BMOL_ARR_LEFT: {
				append_string(ctx, "h%d", segment->dx);
				break;
			}
			case BMOL_ARR_DOWN:
			case BMOL_ARR_UP: {
				append_string(ctx, "v%d", segment->dy);
				break;
			}
			default: {
				break;
			}
		}
	}

	append_string(ctx, "z");
}

size_t bmol_svg_path_len(bmol_outliner* outliner) {
	int len = 0;

	for (int i = 0; i < outliner->segments_size; i++) {
		bmol_path_seg const* segment = &outliner->segments[i];
		int dx = segment->dx;
		int dy = segment->dy;

		switch (segment->type) {
			case BMOL_ARR_NONE: {
				len += 4 + log10_fast(dx > 0 ? dx : 1) + 1 + log10_fast(dy > 0 ? dy : 1) + 1;
				break;
			}
			case BMOL_ARR_RIGHT:
			case BMOL_ARR_LEFT:
			case BMOL_ARR_DOWN:
			case BMOL_ARR_UP: {
				if (dx < 0) {
					dx = -dx;
					len++;
				}

				if (dx > 0) {
					len += 1 + log10_fast(dx) + 1;
				}

				if (dy < 0) {
					dy = -dy;
					len++;
				}

				if (dy > 0) {
					len += 1 + log10_fast(dy) + 1;
				}

				break;
			}
			default: {
				break;
			}
		}
	}

	return len + 1;
}

size_t bmol_svg_path(bmol_outliner* outliner, char buffer[], size_t buf_size) {
	buffer_ctx ctx = {
		.buffer = buffer,
		.buf_size = buf_size,
		.size = 0,
	};

	write_svg(outliner->segments, outliner->segments_size, &ctx);

	return ctx.size;
}

void bmol_set_bitmap(bmol_outliner* outliner, uint8_t const* data) {
	outliner->data = data;
}
