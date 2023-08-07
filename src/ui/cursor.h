#ifndef UI_CURSOR_H
#define UI_CURSOR_H

#include <stdbool.h>
#include <stdint.h>

void xt_cursor_init(void);

// Draws the cursor as a block from (x, y) of size (w x h) using NT1. The cursor
// is drawn using tiles, so while the position is pixel-precise, the size is
// dictated in units of 8x8 blocks.
// Set tile_hl to emphasize one tile within the cursor block's top line, or -1 to not use it.
// Setting line_hl will turn on the line highlight at position y.
void xt_cursor_set(uint16_t x, uint16_t y, int16_t w, int16_t h, int16_t tile_hl, bool line_hl);

// Renders the cursor.
void xt_cursor_update(void);

#endif  // UI_CURSOR_H
