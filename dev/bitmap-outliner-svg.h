#pragma once

/**
 * @file
 *
 * Contains a function to gnerate an SVG path.
 */

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
 * @param buffer The buffer to write the SVG path into.
 * @param buf_size The buffer capacity.
 * @return The length of the generated SVG path.
 */
extern size_t bmol_outliner_svg_path(bmol_outliner* outliner, char buffer[], size_t buf_size);
