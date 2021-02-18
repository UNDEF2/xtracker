#ifndef CGPRINT_H
#define CGPRINT_H

#include <stdint.h>

// cgprint operates on 4bpp data, printing to a 4bpp plane.
// The weird video mode Xtracker sets up means every other pixel is
// actually skipped, so we are able to still do byte-wise addressing
// as if it was in 8bpp mode.

#define CG_ATTR_OPAQUE 0x0100
// cg should point to a 8bpp font bitmap.
// The lower 8 bits of attr are added to the font data.
// The characters should be 5 x 7 px, in the upper-left of each 8 x 8 px tile.
// x and y are in units of 1px.
void cgprint(const uint8_t *cg, int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y);

void cgbox(int16_t plane, uint8_t color, int16_t x1, int16_t y1,
           int16_t x2, int16_t y2);
#endif  // CGPRINT_H
