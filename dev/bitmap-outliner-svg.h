#pragma once

#include "bitmap-outliner.h"

/**
 * Calculate the worst case path length.
 *
 * @return The maximum length of the SVG path string.
 */
extern size_t bmol_outliner_svg_path_len(bmol_outliner* outliner);

/**
 * Create SVG path from segments.
 *
 * @param outliner The outline object.
 * @param out_size The string length.
 * @return The SVG path string.
 */
extern void bmol_outliner_svg_path(bmol_outliner* outliner, char buffer[], size_t buf_size, size_t* out_size);
