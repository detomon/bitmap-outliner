#pragma once

#include <stdint.h>

/**
 * Arrow types.
 */
typedef enum {
	ARROW_NONE = 0,
	ARROW_RIGHT,
	ARROW_LEFT,
	ARROW_DOWN,
	ARROW_UP
} arrow_type;

/**
 * Defines arrow.
 */
typedef struct {
	uint8_t type:3;  ///< Arrow type.
	uint8_t inner:1; ///< Associated path is inner path.
	uint8_t seen:1;  ///< If already seen.
} grid_arrow;

/**
 * Defines path segment.
 */
typedef struct {
	arrow_type type; ///< Arrow type.
	int dx;          ///< Path segment width.
	int dy;          ///< Path segment height.
} path_segment;

/**
 * Defines outliner object.
 */
typedef struct {
	int width;              ///< Bitmap width.
	int height;             ///< Bitmap height.
	uint8_t const* data;    ///< Bitmap data.
	grid_arrow* arrow_grid; ///< Grid arrows.
	path_segment* segments; ///< Path segment buffer.
	int segments_count;     ///< Path segment buffer length.
	int segments_cap;       ///< Path segment buffer capacity.
} outliner;

/**
 * Initialize outliner object.
 *
 * @param outliner The outline object.
 * @param width The bitmap width.
 * @param height The bitmap height.
 * @param data The bitmap data.
 * @return 0 on success.
 */
extern int outliner_init(outliner* object, int width, int height, uint8_t const* data);

/**
 * Free outliner object.
 *
 * @param outliner The outline object.
 */
extern void outliner_free(outliner* outliner);

/**
 * Find paths in bitmap data.
 *
 * @param outliner The outline object.
 * @param out_length The number of path fragments.
 * @return The path fragments.
 */
extern path_segment const* outliner_find_paths(outliner* outliner, int* out_length);
