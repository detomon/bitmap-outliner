#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bitmap-outliner-svg.h"

/**
 * String buffer context.
 */
typedef struct {
	char* buffer;    ///< A string buffer.
	size_t buf_size; ///< The remaining buffer size
	size_t size;     ///< The buffer size.
} bmol_buffer_ctx;

/**
 * Append formated string to buffer.
 *
 * @param buffer_ref A reference to the buffer.
 * @param size_ref A reference to the buffer size.
 * @param format The number format.
 */
static void append_string(bmol_buffer_ctx* ctx, char const* format, ...) {
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
 * Calculate log2 of integer.
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
 *
 * @param n The value to get the log2 from.
 * @return log2 of the given integer.
 */
static unsigned log2_fast(unsigned n) {
	unsigned r;
	unsigned shift;

	r =     (n > 0xFFFF) << 4; n >>= r;
	shift = (n > 0xFF  ) << 3; n >>= shift; r |= shift;
	shift = (n > 0xF   ) << 2; n >>= shift; r |= shift;
	shift = (n > 0x3   ) << 1; n >>= shift; r |= shift;
	                                        r |= (n >> 1);

	return r;
}

/**
 * Calculate log10 of integer.
 *
 * https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
 *
 * @param n The value to get the log10 from.
 * @return log10 of the given integer.
 */
static int log10_fast(unsigned n) {
	static unsigned const pow_10[] = {
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
static void write_svg(bmol_path_seg const* segments, int count, bmol_buffer_ctx* ctx) {
	for (int i = 0; i < count; i++) {
		bmol_path_seg const* segment = &segments[i];

		switch (segment->type) {
			case BMOL_ARR_NONE: {
				if (i > 0) {
					append_string(ctx, "z");
				}

				append_string(ctx, "M %d %d", segment->dx, segment->dy);
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

size_t bmol_outliner_svg_path_len(bmol_outliner* outliner) {
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

size_t bmol_outliner_svg_path(bmol_outliner* outliner, char buffer[], size_t buf_size) {
	bmol_buffer_ctx ctx = {
		.buffer = buffer,
		.buf_size = buf_size,
		.size = 0,
	};

	write_svg(outliner->segments, outliner->segments_size, &ctx);

	return ctx.size;
}
