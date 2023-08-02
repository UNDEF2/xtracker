#include "cgprint.h"
#include <stdio.h>
#include "common.h"
#include "xbase/crtc.h"
#include "xbase/memmap.h"

#define CGPRINT_PLANE_W 512

#define CGPRINT_PLANE_OFFS 0x80000
static uint8_t s_cgfont[8192];

void cgprint_load(const char *fname)
{
	FILE *f = fopen(fname, "rb");
	if (!f)
	{
		fprintf(stderr, "Couldn't open %s\n", fname);
		return;
	}
	fread(s_cgfont, 1, sizeof(s_cgfont), f);
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
	static const int16_t cell_w = 6;
	static const int16_t cell_h = 8;
	volatile uint16_t *dest = get_cgram_ptr(plane, x, y);

	char c;
	while ((c = (*s)))
	{
		const uint8_t *src = &s_cgfont[c * 64];
		for (int j = 0; j < cell_h; j++)
		{
			for (int i = 0; i < cell_w; i++)
			{
				const uint8_t px = src[i];
				volatile uint16_t *dest_local = &dest[i];
				if (px)
				{
					*dest_local = px + (attr & 0x00FF);
				}
				else if (attr & CG_ATTR_OPAQUE)
				{
					*dest_local = 0;
				}
			}
			dest += CGPRINT_PLANE_W;
			src += cell_h;
		}
		dest -= CGPRINT_PLANE_W * cell_h;
		dest += cell_w;
		s++;
	};
}

void cgbox(int16_t plane, uint16_t color, int16_t x1, int16_t y1,
           int16_t x2, int16_t y2)
{
	volatile uint16_t *dest = get_cgram_ptr(plane, x1, y1);

	for (int16_t y = y1; y < y2; y++)
	{
		for (int16_t x = 0; x < x2 - x1; x++)
		{
			dest[x] = color;
		}
		dest += CGPRINT_PLANE_W;
	}
}
