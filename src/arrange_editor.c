#include "arrange_editor.h"
#include <string.h>
#include "ui/fnlabels.h"
#include "ui/metrics.h"
#include "common.h"

void xt_arrange_editor_init(XtArrangeEditor *a, XtArrangeRenderer *r)
{
	memset(a, 0, sizeof(*a));
	a->entry_cursor = -1;
	a->r = r;
}

static void draw_cursor(const XtArrangeEditor *a)
{
//	const int16_t draw_x = XT_UI_ARRANGEMENT_X + XT_UI_ARRANGEMENT_TABLE_OFFS_X + (a->column * XT_UI_COL_SPACING);
//	const int16_t draw_y = XT_UI_ARRANGEMENT_Y + XT_UI_ARRANGEMENT_TABLE_OFFS_Y + (a->frame * XT_UI_ROW_SPACING);
//	xt_cursor_set(draw_x, draw_y, 1, 1, -1, /*line_hl=*/false);
//	xt_cursor_set(32, 32, 4, 4, -1, /*line_hl=*/true);
}

static void push_frames_down(XtTrack *t, int16_t frame)
{
	const int16_t framecnt = t->num_frames < ARRAYSIZE(t->frames)
	                         ? t->num_frames : ARRAYSIZE(t->frames);
	for (int16_t i = framecnt - 2; i >= frame; i--)
	{
		t->frames[i + 1] = t->frames[i];
	}

	if (t->num_frames < ARRAYSIZE(t->frames) - 1) t->num_frames++;
}

static void pull_frames_up(XtTrack *t, int16_t frame)
{
	const int16_t framecnt = t->num_frames < ARRAYSIZE(t->frames)
	                         ? t->num_frames : ARRAYSIZE(t->frames);
	for (int16_t i = frame; i < framecnt - 1; i++)
	{
		t->frames[i] = t->frames[i + 1];
	}

	if (t->num_frames > 1) t->num_frames--;
}

static void swap_frames(XtTrack *t, int16_t frame_a, int16_t frame_b)
{
	XtFrame temp = t->frames[frame_b];
	t->frames[frame_b] = t->frames[frame_a];
	t->frames[frame_a] = temp;
}

static void cursor_down(XtArrangeEditor *a, XtTrack *t)
{
	a->frame++;
	if (a->frame >= t->num_frames) a->frame = 0;
}

static void cursor_up(XtArrangeEditor *a, XtTrack *t)
{
	if (a->frame == 0) a->frame = t->num_frames - 1;
	else a->frame--;
}

static inline void phrase_add(XtArrangeEditor *a, XtTrackRenderer *tr, XtFrame *current, int16_t col, int16_t diff)
{
	current->phrase_id[col] += diff;
	if (current->phrase_id[col] >= XT_PHRASES_PER_CHANNEL) current->phrase_id[col] = 0;
	else if (current->phrase_id[col] < 0) current->phrase_id[col] = 0;

	xt_arrange_renderer_redraw_col(a->r, col);
	xt_track_renderer_repaint_channel(tr, col);
}

void xt_arrange_editor_on_key(XtArrangeEditor *a, XtTrack *t, XtTrackRenderer *tr, XBKeyEvent e)
{
	if (e.modifiers & XB_KEY_MOD_KEY_UP) return;
	XtFrame *current = &t->frames[a->frame];
	switch (e.name)
	{
		// Navigation.
		case XB_KEY_DOWN:
			cursor_down(a, t);
			break;
		case XB_KEY_UP:
			cursor_up(a, t);
			break;
		case XB_KEY_RIGHT:
			a->column++;
			if (a->column >= XT_TOTAL_CHANNEL_COUNT) a->column = 0;
			break;
		case XB_KEY_LEFT:
			if (a->column == 0) a->column = XT_TOTAL_CHANNEL_COUNT - 1;
			else a->column--;
			break;
		// Frame navigation.
		case XB_KEY_R_UP:
			break;
		case XB_KEY_R_DOWN:
			break;
		case XB_KEY_HOME:
			break;
		// Frame change.
		case XB_KEY_OPEN_BRACKET:
		case XB_KEY_NUMPAD_PLUS:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				for (int16_t i = 0; i < ARRAYSIZE(current->phrase_id); i++)
				{
					phrase_add(a, tr, current, i, 1);
				}
			}
			else
			{
				phrase_add(a, tr, current, a->column, 1);
			}
			break;
		case XB_KEY_CLOSED_BRACKET:
		case XB_KEY_NUMPAD_MINUS:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				for (int16_t i = 0; i < ARRAYSIZE(current->phrase_id); i++)
				{
					phrase_add(a, tr, current, i, -1);
				}
			}
			else
			{
				phrase_add(a, tr, current, a->column, -1);
			}
			break;
		case XB_KEY_F1:  // Move -
			if (a->frame == 0) break;
			swap_frames(t, a->frame, a->frame - 1);
			cursor_up(a, t);
			xt_arrange_renderer_request_redraw(a->r, /*content_only=*/true);
			xt_track_renderer_repaint_channel(tr, a->column);
			break;
		case XB_KEY_F2:  // Move +
			swap_frames(t, a->frame, a->frame + 1);
			cursor_down(a, t);
			xt_arrange_renderer_request_redraw(a->r, /*content_only=*/true);
			xt_track_renderer_repaint_channel(tr, a->column);
			break;
		case XB_KEY_F3:  // Copy
			push_frames_down(t, a->frame);
			xt_arrange_renderer_request_redraw(a->r, /*content_only=*/true);
			cursor_down(a, t);
			xt_track_renderer_repaint_channel(tr, a->column);
			break;
		case XB_KEY_F4:  // Add
			push_frames_down(t, a->frame);
			cursor_down(a, t);
			// TODO: Don't use current, maybe. Updating it here is kind of hacky
			current = &t->frames[a->frame];
			// TODO: Should this go to the next "free" phrase ID instead?
			// Depends on phrase count in XtTrackChannelData.
			for (int16_t i = 0; i < XT_TOTAL_CHANNEL_COUNT; i++)
			{
				// Take on the ID of the last frame + 1
				if (a->frame > 0) current->phrase_id[i] = t->frames[a->frame - 1].phrase_id[i] + 1;
				if (current->phrase_id[i] >= XT_PHRASES_PER_CHANNEL) current->phrase_id[i] = 0;  // Wrao
			}
			xt_arrange_renderer_request_redraw(a->r, /*content_only=*/true);
			xt_track_renderer_repaint_channel(tr, a->column);
			break;
		case XB_KEY_F5:  // Del
			pull_frames_up(t, a->frame);
			xt_arrange_renderer_request_redraw(a->r, /*content_only=*/true);
			xt_track_renderer_repaint_channel(tr, a->column);
			break;
		default:
			break;
	}
	draw_cursor(a);
}

void xt_arrange_editor_set_fnlabels(void)
{
	ui_fnlabel_set(0, "Move -");
	ui_fnlabel_set(1, "Move +");
	ui_fnlabel_set(2, "Copy");
	ui_fnlabel_set(3, "Add");
	ui_fnlabel_set(4, "Del");
}
