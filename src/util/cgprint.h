#ifndef UTIL_CGPRINT_H
#define UTIL_CGPRINT_H

#include <stdint.h>

void cgprint_load(const char *fname);

#define CG_ATTR_IGNORE_ALPHA 0x1000
// cg should point to a 8bpp font bitmap.
// The characters should be 5 x 7 px, in the upper-left of each 8 x 8 px tile.
// x and y are in units of 1px.
void cgprint(int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y);

void cgbox(int16_t plane, uint16_t color, int16_t x1, int16_t y1,
           int16_t x2, int16_t y2);
#endif  // CGPRINT_H
