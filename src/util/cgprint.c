#include "util/cgprint.h"
#include <stdio.h>
#include "core/macro.h"
#include "xbase/crtc.h"
#include "xbase/memmap.h"

#include "util/cgprint_sub.h"

#define CGPRINT_PLANE_W 512

#define CGPRINT_PLANE_OFFS 0x80000
static uint8_t s_cgdat[32768];

void cgprint_load(const char *fname)
{
	FILE *f = fopen(fname, "rb");
	if (!f)
	{
		fprintf(stderr, "Couldn't open %s\n", fname);
		return;
	}
	fread(s_cgdat, 1, sizeof(s_cgdat), f);
	fclose(f);
}

static volatile uint16_t *get_cgram_ptr(int16_t plane, int16_t x, int16_t y)
{
	volatile uint16_t *dest = (volatile uint16_t *)XB_GVRAM_BASE +
	                          x + (y * CGPRINT_PLANE_W);
	dest += plane * CGPRINT_PLANE_OFFS/2;
	return dest;
}

void cgprint(int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y)
{
	volatile uint16_t *dest = get_cgram_ptr(plane, x, y);
	cgprint_string_sub(s_cgdat, dest, s, attr);
}

void cgprint_noalpha(int16_t plane, uint32_t colors, const char *s,
                     int16_t x, int16_t y)
{
	volatile uint16_t *dest = get_cgram_ptr(plane, x, y);
	cgprint_string_noalpha_sub(s_cgdat, dest, s, colors);
}

// Draw a CG graphic to the CG plane
void cgtile(int16_t plane, int16_t x, int16_t y,
            uint16_t tile, uint16_t w, uint16_t h)
{
	volatile uint16_t *dest = get_cgram_ptr(plane, x, y);

	cgprint_8x8_sub(&s_cgdat[tile * 64], dest, w, h);
}

void cgbox(int16_t plane, uint8_t attr,
           int16_t x, int16_t y,
           int16_t w, int16_t h)
{
	volatile uint16_t *dest = get_cgram_ptr(plane, x, y);
	attr &= 0x00FF;

	for (int16_t row = 0; row < h; row++)
	{
		cgprint_line_fill_sub(dest, attr, w);
		dest += CGPRINT_PLANE_W;
	}
}
