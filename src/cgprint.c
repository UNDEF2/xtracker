#include "cgprint.h"
#include <stdio.h>
#include "common.h"
#include "x68000/x68k_crtc.h"

static const int16_t s_plane_w = 512;

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

void cgprint(int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y)
{
	static const int16_t cell_w = 6;
	static const int16_t cell_h = 8;
	volatile uint16_t *dest = GVRAM_BASE + (plane * 0x40000) +
	                          x + (y * s_plane_w);
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
			dest += s_plane_w;
			src += cell_h;
		}
		dest -= s_plane_w * cell_h;
		dest += cell_w;
		s++;
	};
}

void cgbox(int16_t plane, uint16_t color, int16_t x1, int16_t y1,
           int16_t x2, int16_t y2)
{
	volatile uint16_t *dest = GVRAM_BASE + (plane * 0x40000) +
	                          x1 + (y1 * s_plane_w);

	for (int16_t y = y1; y < y2; y++)
	{
		for (int16_t x = 0; x < x2 - x1; x++)
		{
			dest[x] = color;
		}
		dest += s_plane_w;
	}
}
