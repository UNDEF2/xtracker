#include "ui/arrange_render.h"
#include <string.h>

#include "cgprint.h"
#include "common.h"
#include "xt_palette.h"

static const int16_t krow_spacing = 8;
static const int16_t kcol_spacing = 16;

// Set up the XtArrange struct for its first render and further use.
void xt_arrange_renderer_init(XtArrangeRenderer *a, const XtTrack *t)
{
	memset(a, 0, sizeof(*a));
	for (int16_t i = 0; i < ARRAYSIZE(a->frames); i++)
	{
		for (int16_t j = 0; j < ARRAYSIZE(a->frames[0].phrase_id); j++)
		{
			a->frames[i].phrase_id[j] = -1;
		}
	}

	a->draw_x = XT_ARRANGE_RENDER_X;
	a->draw_y = XT_ARRANGE_RENDER_Y;
}

// Draw the arrangement table as-needed based on track data and the provided
// navigation position.
void xt_arrange_renderer_tick(XtArrangeRenderer *a, const XtTrack *t,
                            int16_t row, int16_t col)
{
	if (!a->border_drawn)
	{
		static const char *left_mapping[] =
		{
			"\x05", "\x03", "\x03", "\x03", "\x03", "\x03", "\x07"
		};
		static const char *right_mapping[] =
		{
			"\x04", "\x03", "\x03", "\x03", "\x03", "\x03", "\x06"
		};
		int16_t y = a->draw_y;
		const int16_t left_x = a->draw_x;
		const int16_t right_x = left_x +
		                        (kcol_spacing * ARRAYSIZE(a->frames[0].phrase_id)) + 2;
		cgprint(0, XT_PAL_UI_BORDER,"Arrangement", left_x + 6, y);
		y += krow_spacing;

		for (int16_t i = 0; i < ARRAYSIZE(a->frames); i++)
		{
			cgprint(0, XT_PAL_UI_BORDER, left_mapping[i], left_x, y);
			cgprint(0, XT_PAL_UI_BORDER, right_mapping[i], right_x, y);
			y += krow_spacing;
		}

		a->border_drawn = true;
	}

	if (row >= 0)
	{
		a->row = row;
	}
	if (col >= 0)
	{
		a->column = col;
	}

	int16_t cell_y = a->draw_y + krow_spacing;

	// Look for data that needs an update.
	for (int16_t i = 0; i < ARRAYSIZE(a->frames); i++)
	{
		XtFrame *current = &a->frames[i];
		const int16_t ref_frame_id = a->row - (ARRAYSIZE(a->frames) / 2) + i;
		const XtFrame *ref = NULL;
		if (ref_frame_id >= 0 && ref_frame_id < t->num_frames)
		{
			ref = &t->frames[ref_frame_id];
		}

		int16_t cell_x = a->draw_x + 6;

		// now compare the reference frame to what we've already drawn.
		for (int16_t j = 0; j < ARRAYSIZE(current->phrase_id); j++)
		{
			if (ref)
			{
				if (current->phrase_id[j] == ref->phrase_id[j] && j != col)
				{
					cell_x += kcol_spacing;
					continue;
				}
				current->phrase_id[j] = ref->phrase_id[j];
			}
			else
			{
				if (current->phrase_id[j] == -1)
				{
					cell_x += kcol_spacing;
					continue;
				}
				current->phrase_id[j] = -1;
			}

			// Redraw the current phrase id.
			if (current->phrase_id[j] == -1)
			{
				cgbox(0, XT_PAL_TRANSPARENT, cell_x, cell_y, cell_x + 12, cell_y + krow_spacing);
			}
			else
			{
				const int16_t pal = (j == a->column && i == (ARRAYSIZE(a->frames) / 2)) ?
				                    XT_PAL_UI_HIGHLIGHT_FG :
				                    XT_PAL_UI_FG;
				const uint8_t id = current->phrase_id[j];
				char buffer[3] = {0, 0, 0};
				buffer[0] = 0x10 | ((id & 0xF0) >> 4);
				buffer[1] = 0x10 | (id & 0x0F);
				cgprint(0, pal | CG_ATTR_OPAQUE, buffer, cell_x, cell_y);
				// Highlighted cells get marked for potential repaint
				if (pal == XT_PAL_UI_HIGHLIGHT_FG)
				{
					current->phrase_id[j] = -1;
				}
			}

			cell_x += kcol_spacing;
		}
		cell_y += krow_spacing;
	}
}

// Mark all frames and the border as needing a redraw.
void xt_arrange_renderer_redraw(XtArrangeRenderer *a)
{
	xt_arrange_renderer_init(a, NULL);
}


