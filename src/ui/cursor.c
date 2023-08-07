#include "ui/cursor.h"
#include <string.h>

#include <xbase/pcg.h>

#define XT_CURSOR_HL_PAL 1

typedef struct XtUiCursorState
{
	int16_t x;  // Pixel units.
	int16_t y;
	int16_t w;  // Tile units.
	int16_t h;
	int16_t tile_hl;
	bool line_hl;
} XtUiCursorState;

XtUiCursorState s_state;
XtUiCursorState s_state_prev;


void xt_cursor_init(void)
{
	memset(&s_state, 0, sizeof(s_state));
	memset(&s_state_prev, 0, sizeof(s_state_prev));

	volatile uint16_t *nt1 = (volatile uint16_t *)XB_PCG_BG1_NAME;
	for (int16_t ty = 0; ty < 512/8; ty++)
	{
		for (int16_t tx = 0; tx < 512/8; tx++)
		{
			*nt1++ = 0;
		}
	}

}

void xt_cursor_set(int16_t x, int16_t y, int16_t w, int16_t h, int16_t tile_hl, bool line_hl)
{
	s_state.x = x;
	s_state.y = y;
	s_state.w = w;
	s_state.h = h;
	s_state.tile_hl = tile_hl;
	s_state.line_hl = line_hl;
}

void xt_cursor_update(void)
{
	// Erase the cursor from before.
	volatile uint16_t *nt1 = (volatile uint16_t *)XB_PCG_BG1_NAME;
	for (int16_t ty = 0; ty < s_state_prev.h; ty++)
	{
		volatile uint16_t *nt_line = &nt1[ty * 512/8];
		for (int16_t tx = 0; tx < s_state_prev.w; tx++)
		{
			*nt_line++ = 0;
		}
	}

	// Draw the full line highlight.
	if (s_state.line_hl && !s_state_prev.line_hl)
	{
		volatile uint16_t *nt_line = nt1;
		for (int16_t tx = 0; tx < 512/8; tx++)
		{
			*nt_line++ = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x80);
		}
	}
	else if (!s_state.line_hl && s_state_prev.line_hl) // or erase it
	{
		volatile uint16_t *nt_line = nt1;
		for (int16_t tx = 0; tx < 512/8; tx++)
		{
			*nt_line++ = 0;
		}
	}

	// Draw the line highlight in the area erased before regardless of the
	// line status of the last frame.
	if (s_state.line_hl)
	{
		volatile uint16_t *nt_line = &nt1[s_state_prev.w];
		for (int16_t tx = s_state_prev.w; tx < 512/8; tx++)
		{
			*nt_line++ = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x80);
		}
	}

	// Draw the cursor block
	if (s_state.tile_hl < 0)
	{
		for (int16_t ty = 0; ty < s_state.h; ty++)
		{
			for (int16_t tx = 0; tx < s_state.w; tx++)
			{
				*nt1++ = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x81);
			}
		}
	}
	else
	{
		for (int16_t ty = 0; ty < s_state.h; ty++)
		{
			for (int16_t tx = 0; tx < s_state.w; tx++)
			{
				*nt1++ = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, (tx == s_state.tile_hl) ? 0x81 : 0x82);
			}
		}
	}

	s_state_prev = s_state;

	xb_pcg_set_bg1_xscroll(-s_state.x);
	xb_pcg_set_bg1_yscroll(-s_state.y);
}
