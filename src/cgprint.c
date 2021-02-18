#include "cgprint.h"
#include "x68000/x68k_crtc.h"

static const int16_t stride = 2;
static const int16_t plane_w = 1024;

void cgprint(const uint8_t *cg, int16_t plane, uint16_t attr, const char *s,
             int16_t x, int16_t y)
{
	static const int16_t cell_w = 6;
	static const int16_t cell_h = 8;
	volatile uint8_t *dest = GVRAM_BASE + (plane * 0x80000) +
	                         (stride * x) + (y * plane_w);
	if (!*s)
	{
		return;
	}
	do
	{
		const char c = *s++;
		const uint8_t *src = &cg[c * 64];
		for (int j = 0; j < cell_h; j++)
		{
			for (int i = 0; i < cell_w; i++)
			{
				const uint8_t px = src[i];
				volatile uint8_t *dest_local = &dest[1 + (i * stride)];
				if (px)
				{
					*dest_local = px + (attr & 0x00FF);
				}
				else if (attr & CG_ATTR_OPAQUE)
				{
					*dest_local = 0;
				}
			}
			dest += plane_w;
			src += cell_h;
		}
		dest -= plane_w * cell_h;
		dest += stride * cell_w;
	} while (*s);
}

void cgbox(int16_t plane, uint8_t color, int16_t x1, int16_t y1,
           int16_t x2, int16_t y2)
{
	volatile uint8_t *dest = GVRAM_BASE + (plane * 0x80000) +
	                         (stride * x1) + (y1 * plane_w);

	for (int16_t y = y1; y < y2; y++)
	{
		for (int16_t x = 0; x < x2 - x1; x++)
		{
			dest[1 + (x * stride)] = color;
		}
		dest += plane_w;
	}
}
