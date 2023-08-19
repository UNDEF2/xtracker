#include "arrange_editor.h"
#include <string.h>
#include "ui/fnlabels.h"
#include "common.h"

void xt_arrange_editor_init(XtArrangeEditor *a, XtArrangeRenderer *r)
{
	memset(a, 0, sizeof(*a));
	a->entry_cursor = -1;
	a->r = r;
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

void xt_arrange_editor_on_key(XtArrangeEditor *a, XtTrack *t, XtKeyEvent e)
{
	XtFrame *current = &t->frames[a->frame];
	switch (e.name)
	{
		// Navigation.
		case XT_KEY_DOWN:
			cursor_down(a, t);
			break;
		case XT_KEY_UP:
			cursor_up(a, t);
			break;
		case XT_KEY_RIGHT:
			a->column++;
			if (a->column >= XT_TOTAL_CHANNEL_COUNT) a->column = 0;
			break;
		case XT_KEY_LEFT:
			if (a->column == 0) a->column = XT_TOTAL_CHANNEL_COUNT - 1;
			else a->column--;
			break;
		// Frame navigation.
		case XT_KEY_R_UP:
			break;
		case XT_KEY_R_DOWN:
			break;
		case XT_KEY_HOME:
			break;
		// Frame change.
		case XT_KEY_NUMPAD_PLUS:
			current->phrase_id[a->column]++;
			if (current->phrase_id[a->column] >= XT_PHRASES_PER_CHANNEL)
			{
				current->phrase_id[a->column] = 0;
			}
			xt_arrange_renderer_redraw_col(a->r, a->column);
			break;
		case XT_KEY_NUMPAD_MINUS:
			if (current->phrase_id[a->column] == 0)
			{
				current->phrase_id[a->column] = XT_PHRASES_PER_CHANNEL - 1;
			}
			else
			{
				current->phrase_id[a->column]--;
			}
			xt_arrange_renderer_redraw_col(a->r, a->column);
			break;
		case XT_KEY_F1:  // Move -
			if (a->frame == 0) break;
			swap_frames(t, a->frame, a->frame - 1);
			cursor_up(a, t);
			xt_arrange_renderer_redraw(a->r);
			break;
		case XT_KEY_F2:  // Move +
			swap_frames(t, a->frame, a->frame + 1);
			cursor_down(a, t);
			xt_arrange_renderer_redraw(a->r);
			break;
		case XT_KEY_F3:  // Copy
			push_frames_down(t, a->frame);
			xt_arrange_renderer_redraw(a->r);
			cursor_down(a, t);
			break;
		case XT_KEY_F4:  // Add
			push_frames_down(t, a->frame);
			cursor_down(a, t);
			// TODO: Don't use current, maybe. Updating it here is kind of hacky
			current = &t->frames[a->frame]
			for (int16_t i = 0; i < XT_TOTAL_CHANNEL_COUNT; i++)
			{
				if (current->phrase_id[i] < XT_PHRASES_PER_CHANNEL - 1) current->phrase_id[i]++;
			}
			xt_arrange_renderer_redraw(a->r);
			break;
		case XT_KEY_F5:  // Del
			pull_frames_up(t, a->frame);
			xt_arrange_renderer_redraw(a->r);
			break;
		default:
			break;
	}
}

void xt_arrange_editor_set_fnlabels(void)
{
	ui_fnlabel_set(0, "Move -");
	ui_fnlabel_set(1, "Move +");
	ui_fnlabel_set(2, "Copy");
	ui_fnlabel_set(3, "Add");
	ui_fnlabel_set(4, "Del");
}
