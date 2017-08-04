#pragma once

#include <stdint.h>

/**
 * Arrow types.
 */
typedef enum {
	BMOL_ARR_NONE = 0,
	BMOL_ARR_RIGHT,
	BMOL_ARR_LEFT,
	BMOL_ARR_DOWN,
	BMOL_ARR_UP
} bmol_arr_type;

/**
 * Defines arrow.
 */
typedef struct {
	uint8_t type:3;     ///< Arrow type.
	uint8_t inner:1;    ///< Associated path is inner path.
	uint8_t seen:1;     ///< Has been seen.
	uint8_t visited:1;  ///< Has been visited.
} bmol_arrow;

/**
 * Defines path segment.
 */
typedef struct {
	uint8_t type; ///< Arrow type.
	int dx;       ///< Path segment width.
	int dy;       ///< Path segment height.
} bmol_path_seg;

/**
 * Defines outliner object.
 */
typedef struct {
	int width;               ///< Bitmap width.
	int height;              ///< Bitmap height.
	uint8_t const* data;     ///< Bitmap data.
	bmol_arrow* arrow_grid;  ///< Grid arrows.
	bmol_path_seg* segments; ///< Path segment buffer.
	int segments_count;      ///< Path segment buffer length.
	int segments_cap;        ///< Path segment buffer capacity.
} bmol_outliner;

/**
 * Initialize outliner object.
 *
 * @param outliner The outline object.
 * @param width The bitmap width.
 * @param height The bitmap height.
 * @param data The bitmap data.
 * @return 0 on success.
 */
extern int bmol_init(bmol_outliner* object, int width, int height, uint8_t const* data);

/**
 * Free outliner object.
 *
 * @param outliner The outline object.
 */
extern void bmol_free(bmol_outliner* outliner);

/**
 * Find paths in bitmap data.
 *
 * @param outliner The outline object.
 * @param out_length The number of path fragments.
 * @return The path fragments.
 */
extern bmol_path_seg const* bmol_outliner_find_paths(bmol_outliner* outliner, int* out_length);
