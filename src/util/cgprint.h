#ifndef UTIL_CGPRINT_H
#define UTIL_CGPRINT_H

#include <stdint.h>
#include "palette.h"
#include "ui/metrics.h"

void cgprint_load(const char *fname);

// The characters lie within a 6 x 8 area, contained within an 8 x 8 px tile.
void cgprint(int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y);

// Draw a CG graphic to the CG plane
void cgtile(int16_t plane, int16_t x, int16_t y,
            uint16_t tile, uint16_t w, uint16_t h);

void cgbox(int16_t plane, uint8_t attr, int16_t x, int16_t y,
           int16_t w, int16_t h);

// Little case-specific helpers.
static inline void cgprint_hex1(uint8_t plane, uint8_t pal, uint16_t x, uint16_t y, uint8_t val)
{
	cgbox(XT_UI_PLANE, XT_PAL_BACK, x, y, 6, 7);
	char buffer[2] = {0};
	buffer[0] = 0x10 | (val & 0x0F);
	cgprint(plane, pal, buffer, x, y);
}

static inline void cgprint_hex2(uint8_t plane, uint8_t pal, uint16_t x, uint16_t y, uint8_t val)
{
	cgbox(XT_UI_PLANE, XT_PAL_BACK, x, y, 12, 7);
	char buffer[3] = {0};
	buffer[0] = 0x10 | ((val & 0xF0) >> 4);
	buffer[1] = 0x10 | (val & 0x0F);
	cgprint(plane, pal, buffer, x, y);
}
#endif  // CGPRINT_H
