#include "ui/arrange_render.h"
#include <string.h>

#include "util/cgprint.h"
#include "common.h"
#include "xt_palette.h"

#include "ui/metrics.h"

// Set up the XtArrange struct for its first render and further use.
void xt_arrange_renderer_init(XtArrangeRenderer *a)
{
	memset(a, 0, sizeof(*a));
	for (int16_t i = 0; i < ARRAYSIZE(a->frames); i++)
	{
		for (int16_t j = 0; j < ARRAYSIZE(a->frames[0].phrase_id); j++)
		{
			a->frames[i].phrase_id[j] = -1;
		}
		a->frame_id[i] = -1;
	}
}

// Draw the arrangement table as-needed based on track data and the provided
// navigation position.
void xt_arrange_renderer_tick(XtArrangeRenderer *a, const XtTrack *t,
                            int16_t row, int16_t col)
{
	const int16_t kbase_x = XT_UI_ARRANGEMENT_X;
	const int16_t kbase_y = XT_UI_ARRANGEMENT_Y;
	const int16_t kbase_w = XT_UI_AREA_W - kbase_x - XT_UI_MARGIN_SIDE;
	const int16_t kbase_h = XT_UI_AREA_H - kbase_y - 8;  // Space for the channel indicators
	if (!a->backing_drawn)
	{
		// Back black rectangle area.
		cgbox(XT_UI_PLANE, XT_PAL_BACK, kbase_x, kbase_y, kbase_w, kbase_h);

		// Title.
		cgprint(XT_UI_PLANE, XT_PAL_ACCENT1, "Arrangement Table",
		        kbase_x + XT_UI_TITLEBAR_TEXT_OFFS_X,
		        kbase_y + XT_UI_TITLEBAR_TEXT_OFFS_Y);

		// Accent line (vert)
		cgbox(XT_UI_PLANE, XT_PAL_ACCENT2,
		      kbase_x, kbase_y,
		      1, XT_UI_TITLEBAR_LINE_H);
		// Accent line (horiz)
		cgbox(XT_UI_PLANE, XT_PAL_ACCENT2,
		      kbase_x, kbase_y + XT_UI_TITLEBAR_LINE_H,
		      kbase_w, 1);

		a->backing_drawn = true;
	}

	if (row >= 0)
	{
		a->edit.row = row;
	}
	if (col >= 0)
	{
		a->edit.column = col;
	}

	int16_t cell_y = XT_UI_ARRANGEMENT_Y + XT_UI_TITLEBAR_CONTENT_OFFS_Y;

	// Look for data that needs an update.
	for (int16_t i = 0; i < ARRAYSIZE(a->frames); i++)
	{
		XtFrame *current = &a->frames[i];
		const int16_t ref_frame_id = a->edit.row - (ARRAYSIZE(a->frames) / 2) + i;
		const XtFrame *ref = NULL;
		if (ref_frame_id >= 0 && ref_frame_id < t->num_frames)
		{
			ref = &t->frames[ref_frame_id];
		}

		// If the frame count changed, invalidate all of the frame ID gutter
		if (a->last_frame_count != t->num_frames)
		{
			cgbox(XT_UI_PLANE, XT_PAL_BACK, XT_UI_ARRANGEMENT_X, cell_y,
			      XT_UI_COL_SPACING, XT_UI_ROW_SPACING);
			a->frame_id[i] = -1;
		}

		int16_t cell_x = XT_UI_ARRANGEMENT_X + XT_UI_ARRANGEMENT_TABLE_OFFS_X;

		// now compare the reference frame to what we've already drawn.
		for (int16_t j = 0; j < ARRAYSIZE(current->phrase_id); j++)
		{
			if (ref)
			{
				if (current->phrase_id[j] == ref->phrase_id[j] && j != col)
				{
					cell_x += XT_UI_COL_SPACING;
					continue;
				}
				current->phrase_id[j] = ref->phrase_id[j];
			}
			else
			{
				if (current->phrase_id[j] == -1)
				{
					// If the frame count has changed since last time,
					// erase inactive areas.
					if (a->last_frame_count != t->num_frames)
					{
						cgbox(XT_UI_PLANE, XT_PAL_BACK, cell_x, cell_y,
						      XT_UI_COL_SPACING, XT_UI_ROW_SPACING);
					}

					cell_x += XT_UI_COL_SPACING;
					continue;
				}
				current->phrase_id[j] = -1;
			}

			// Redraw the current phrase id.
			if (current->phrase_id[j] == -1)
			{
				cgbox(XT_UI_PLANE, XT_PAL_BACK, cell_x, cell_y,
				      XT_UI_COL_SPACING, XT_UI_ROW_SPACING);
			}
			else
			{
				// Choose palette to highlight
				const bool is_editor_col = j == a->edit.column;
				const bool is_center_row = i == (ARRAYSIZE(a->frames) / 2);
				int16_t pal = XT_PAL_INACTIVE;
				if (is_center_row) pal = XT_PAL_ACCENT2;
				if (is_editor_col && is_center_row) pal = XT_PAL_MAIN;  // TODO: Don't do this if editor is inactive?

				// hex print for phrase ID
				const uint8_t id = current->phrase_id[j];
				char buffer[3] = {0};
				buffer[0] = 0x10 | ((id & 0xF0) >> 4);
				buffer[1] = 0x10 | (id & 0x0F);
				cgbox(XT_UI_PLANE, XT_PAL_BACK, cell_x, cell_y,
				      XT_UI_COL_SPACING, XT_UI_ROW_SPACING);
				cgprint(XT_UI_PLANE, pal, buffer, cell_x, cell_y);
				// Highlighted cells get marked for potential repaint
				if (is_editor_col && is_center_row) current->phrase_id[j] = -1;
			}

			cell_x += XT_UI_COL_SPACING;
		}

		// Print the frame ID.
		if (ref)
		{
			if (ref_frame_id != a->frame_id[i])
			{
				a->frame_id[i] = ref_frame_id;
				char buffer[4] = "  :";
				buffer[0] = 0x10 | ((ref_frame_id & 0xF0) >> 4);
				buffer[1] = 0x10 | (ref_frame_id & 0x0F);
				cgbox(XT_UI_PLANE, XT_PAL_BACK, XT_UI_ARRANGEMENT_X, cell_y,
				      XT_UI_COL_SPACING, XT_UI_ROW_SPACING);
				cgprint(XT_UI_PLANE, XT_PAL_ACCENT1, buffer, XT_UI_ARRANGEMENT_X, cell_y);
			}
		}
		else if (a->frame_id[i] != -1)
		{
			cgbox(XT_UI_PLANE, XT_PAL_BACK, XT_UI_ARRANGEMENT_X, cell_y,
			      XT_UI_COL_SPACING, XT_UI_ROW_SPACING);
			a->frame_id[i] = -1;
		}

		cell_y += XT_UI_ROW_SPACING;
	}

	a->last_frame_count = t->num_frames;
}

// Mark all frames and the border as needing a redraw.
void xt_arrange_renderer_request_redraw(XtArrangeRenderer *a, bool content_only)
{
	xt_arrange_renderer_init(a);
	if (content_only) a->backing_drawn = true;
}

void xt_arrange_renderer_redraw_col(XtArrangeRenderer *a, int16_t col)
{
	for (int16_t i = 0; i < ARRAYSIZE(a->frames); i++)
	{
		a->frames[i].phrase_id[col] = -1;
	}
}
