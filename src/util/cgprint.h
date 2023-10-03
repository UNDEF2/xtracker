#ifndef UTIL_CGPRINT_H
#define UTIL_CGPRINT_H

#include <stdint.h>

void cgprint_load(const char *fname);

// The characters lie within a 6 x 8 area, contained within an 8 x 8 px tile.
void cgprint(int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y);

// Draw a CG graphic to the CG plane
void cgtile(int16_t plane, int16_t x, int16_t y,
            uint16_t tile, uint16_t w, uint16_t h);

void cgbox(int16_t plane, uint8_t attr, int16_t x, int16_t y,
           int16_t w, int16_t h);
#endif  // CGPRINT_H
