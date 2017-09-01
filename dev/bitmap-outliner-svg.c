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

void bmol_outliner_svg_path(bmol_outliner* outliner, char buffer[], size_t buf_size, size_t* out_size) {
	bmol_buffer_ctx ctx = {
		.buffer = buffer,
		.buf_size = buf_size,
		.size = 0,
	};

	write_svg(outliner->segments, outliner->segments_size, &ctx);
	*out_size = ctx.size;
}
