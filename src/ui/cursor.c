#include "ui/cursor.h"
#include <string.h>

#include <xbase/pcg.h>

#include "ui/track_render.h"

#define XT_CURSOR_HL_PAL 1

typedef struct XtUiCursorState
{
	uint16_t x;  // Pixel units.
	uint16_t y;
	int16_t w;  // Tile units.
	int16_t h;
	int16_t tile_hl;
	bool line_hl;
	bool dirty;
} XtUiCursorState;

XtUiCursorState s_state;
XtUiCursorState s_state_prev;


void xt_cursor_init(void)
{
	memset(&s_state, 0, sizeof(s_state));
	memset(&s_state_prev, 0, sizeof(s_state_prev));

	volatile uint16_t *nt1 = (volatile uint16_t *)XB_PCG_BG1_NAME;
	for (int16_t ty = 0; ty < XT_RENDER_NT_CHARS; ty++)
	{
		for (int16_t tx = 0; tx < XT_RENDER_NT_CHARS; tx++)
		{
			*nt1++ = 0;
		}
	}
}

void xt_cursor_set(uint16_t x, uint16_t y, int16_t w, int16_t h, int16_t tile_hl, bool line_hl)
{
	if (w <= 0) w = 1;
	if (h <= 0) h = 1;
	s_state.x = x % (XT_RENDER_NT_CHARS * XT_RENDER_CELL_W_PIXELS);
	s_state.y = y % (XT_RENDER_NT_CHARS * XT_RENDER_CELL_H_PIXELS);
	s_state.w = w;
	s_state.h = h;
	s_state.tile_hl = tile_hl;
	s_state.line_hl = line_hl;
	s_state.dirty = true;
}

static void draw_rect(uint16_t x, uint16_t y, int16_t w, int16_t h, uint16_t val)
{
	volatile uint16_t *nt1 = (volatile uint16_t *)XB_PCG_BG1_NAME;
	if (s_state.tile_hl < 0)
	{
		for (int16_t ty = 0; ty < s_state.h; ty++)
		{
			volatile uint16_t *nt_line = &nt1[ty * XT_RENDER_NT_CHARS];
			for (int16_t tx = 0; tx < s_state.w; tx++)
			{
				*nt_line++ = val;
			}
		}
	}
}

void xt_cursor_update(void)
{
	if (!s_state.dirty) return;
	s_state.dirty = false;

	// Erase the cursor from before.
	volatile uint16_t *nt1 = (volatile uint16_t *)XB_PCG_BG1_NAME;
	for (int16_t ty = 0; ty < s_state_prev.h; ty++)
	{
		volatile uint16_t *nt_line = &nt1[ty * XT_RENDER_NT_CHARS];
		for (int16_t tx = 0; tx < s_state_prev.w; tx++)
		{
			*nt_line++ = 0;
		}
	}

	// Draw the full line highlight.
	if (s_state.line_hl && !s_state_prev.line_hl)
	{
		volatile uint16_t *nt_line = nt1;
		for (int16_t tx = 0; tx < XT_RENDER_NT_CHARS; tx++)
		{
			*nt_line++ = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x80);
		}
	}
	else if (!s_state.line_hl && s_state_prev.line_hl) // or erase it
	{
		volatile uint16_t *nt_line = nt1;
		for (int16_t tx = 0; tx < XT_RENDER_NT_CHARS; tx++)
		{
			*nt_line++ = 0;
		}
	}

	// Draw the line highlight in the area erased before regardless of the
	// line status of the last frame.
	if (s_state.line_hl)
	{
		volatile uint16_t *nt_line = &nt1[s_state.w];
		for (int16_t tx = s_state.w; tx < s_state_prev.w; tx++)
		{
			*nt_line++ = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x80);
		}
	}

	// Draw the cursor block
	if (s_state.tile_hl < 0)
	{
		draw_rect(0, s_state.w, 0, s_state.h, XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x81));
	}
	else
	{
		draw_rect(0, s_state.w, 0, s_state.h, XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x82));
		volatile uint16_t *nt_tile = &nt1[s_state.tile_hl];
		*nt_tile = XB_PCG_ATTR(0, 0, XT_CURSOR_HL_PAL, 0x81);
	}

	s_state_prev = s_state;

	xb_pcg_set_bg1_xscroll(-s_state.x);
	xb_pcg_set_bg1_yscroll(-s_state.y);
}
