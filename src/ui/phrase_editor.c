#include "ui/phrase_editor.h"
#include "xt_keys.h"
#include "ui/cursor.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <dos.h>

#define ROLL_SCROLL_MAGNITUDE 16

void xt_phrase_editor_init(XtPhraseEditor *p, const XtTrack *t)
{
	memset(p, 0, sizeof(*p));
	p->state = EDITOR_NORMAL;
	p->visible_channels = XT_RENDER_VISIBLE_CHANNELS;
	p->octave = 3;

	// TODO: Respect dynamic channel data
	p->total_channels = ARRAYSIZE(t->channel_data);
}

// ============================================================================
// Camera updates.
// ============================================================================

int16_t xt_phrase_editor_get_cam_x(const XtPhraseEditor *p)
{
	return XT_RENDER_CELL_CHARS * p->cam_column * XT_RENDER_CELL_W_PIXELS;
}

int16_t xt_phrase_editor_get_cam_y(const XtPhraseEditor *p)
{
	return -128 + (p->row * XT_RENDER_CELL_H_PIXELS);
}

void xt_phrase_editor_update_renderer(XtPhraseEditor *p, XtTrackRenderer *r)
{
	xt_track_renderer_set_camera(r, xt_phrase_editor_get_cam_x(p),
	                             xt_phrase_editor_get_cam_y(p));
	for (int i = 0; i < ARRAYSIZE(p->channel_dirty); i++)
	{
		if (p->channel_dirty[i])
		{
			xt_track_renderer_repaint_channel(r, i);
			p->channel_dirty[i] = false;
		}
	}
}

// ============================================================================
// Cursor movement.
// ============================================================================

static void frame_down(XtPhraseEditor *p, const XtTrack *t)
{
	if (p->frame >= t->num_frames - 1) p->frame = 0;
	else p->frame++;
}

static void frame_up(XtPhraseEditor *p, const XtTrack *t)
{
	if (p->frame <= 0) p->frame = t->num_frames - 1;
	else p->frame--;
}

static void cursor_down(XtPhraseEditor *p, const XtTrack *t, bool allow_frame)
{
	if (p->row >= t->phrase_length - 1)
	{
		if (allow_frame)
		{
			p->row = 0;
			frame_down(p, t);
		}
	}
	else
	{
		p->row++;
	}
}

static void cursor_up(XtPhraseEditor *p, const XtTrack *t, bool allow_frame)
{
	if (p->row <= 0)
	{
		if (allow_frame)
		{
			p->row = t->phrase_length - 1;
			frame_up(p, t);
		}
	}
	else
	{
		p->row--;
	}
}

static bool cursor_column_right(XtPhraseEditor *p, const XtTrack *t, bool wrap)
{
	p->column++;
	// TODO: Respect dynamic channel count
	if (p->column >= ARRAYSIZE(t->channel_data))
	{
		p->column = (wrap ? 0 : p->column - 1);
		return wrap;
	}
	return true;
}

static void cursor_right(XtPhraseEditor *p, const XtTrack *t, bool wrap)
{
	p->sub_pos++;
	if (p->sub_pos >= CURSOR_SUBPOS_MAX_INVALID)
	{
		if (cursor_column_right(p, t, wrap)) p->sub_pos = 0;
		else p->sub_pos--;
	}
}

static bool cursor_column_left(XtPhraseEditor *p, const XtTrack *t, bool wrap)
{
	if (p->column <= 0)
	{
		if (wrap) p->column = ARRAYSIZE(t->channel_data) - 1;
		return wrap;
	}
	else p->column--;
	return true;
}

static void cursor_left(XtPhraseEditor *p, const XtTrack *t, bool wrap)
{
	if (p->sub_pos <= 0)
	{
		if (cursor_column_left(p, t, wrap)) p->sub_pos = CURSOR_SUBPOS_MAX_INVALID - 1;
	}
	else
	{
		p->sub_pos--;
	}
}

// ============================================================================
// Cursor drawing routines.
// ============================================================================
static inline uint16_t get_x_for_column(uint16_t column,
                                        XtEditorCursorSubPos sub_pos)
{
	const uint16_t base = (XT_RENDER_CELL_CHARS * XT_RENDER_CELL_W_PIXELS) * column;
	switch (sub_pos)
	{
		default:
			return 0;
		case CURSOR_SUBPOS_NOTE:
			return base;
		case CURSOR_SUBPOS_INSTRUMENT_HIGH:
			return base + 3 * XT_RENDER_CELL_W_PIXELS;
		case CURSOR_SUBPOS_INSTRUMENT_LOW:
			return base + 4 * XT_RENDER_CELL_W_PIXELS;
		case CURSOR_SUBPOS_CMD1:
			return base + 5 * XT_RENDER_CELL_W_PIXELS;
		case CURSOR_SUBPOS_ARG1_HIGH:
			return base + 6 * XT_RENDER_CELL_W_PIXELS;
		case CURSOR_SUBPOS_ARG1_LOW:
			return base + 7 * XT_RENDER_CELL_W_PIXELS;
	}
}

// Let the horizontal column get "pushed" by the by the cursor.
static void cursor_update_cam_column(XtPhraseEditor *p)
{
	const int16_t right_margin = p->visible_channels - 1;
	if (p->column < p->cam_column)
	{
		p->cam_column = p->column;
	}
	else if (p->column > p->cam_column + right_margin)
	{
		p->cam_column = p->column - right_margin;
	}
}

static void draw_normal_cursor(const XtPhraseEditor *p)
{
	int16_t draw_x = get_x_for_column(p->column, p->sub_pos) - xt_phrase_editor_get_cam_x(p);
	const int16_t draw_y = (p->row * XT_RENDER_CELL_H_PIXELS) - xt_phrase_editor_get_cam_y(p);

	const bool line_hl = true;

	switch (p->sub_pos)
	{
		default:
			break;
		case CURSOR_SUBPOS_NOTE:
			xt_cursor_set(draw_x, draw_y, 3, 1, -1, line_hl);
			break;
		case CURSOR_SUBPOS_INSTRUMENT_HIGH:
			xt_cursor_set(draw_x, draw_y, 2, 1, 0, line_hl);
			break;
		case CURSOR_SUBPOS_INSTRUMENT_LOW:
			draw_x -= 8;
			xt_cursor_set(draw_x, draw_y, 2, 1, 1, line_hl);
			break;
		case CURSOR_SUBPOS_CMD1:
			xt_cursor_set(draw_x, draw_y, 3, 1, 0, line_hl);
			break;
		case CURSOR_SUBPOS_ARG1_HIGH:
			draw_x -= 8;
			xt_cursor_set(draw_x, draw_y, 3, 1, 1, line_hl);
			break;
		case CURSOR_SUBPOS_ARG1_LOW:
			draw_x -= 16;
			xt_cursor_set(draw_x, draw_y, 3, 1, 2, line_hl);
			break;
	}
}

static void draw_select_region(const XtPhraseEditor *p)
{
	int16_t from_x = get_x_for_column(p->select.from_column, p->select.from_sub_pos) - xt_phrase_editor_get_cam_x(p);
	int16_t to_x = get_x_for_column(p->column, p->sub_pos) - xt_phrase_editor_get_cam_x(p);

	if (from_x <= to_x && p->sub_pos == CURSOR_SUBPOS_NOTE) to_x += 2 * XT_RENDER_CELL_W_PIXELS;
	else if (from_x > to_x && p->select.from_sub_pos == CURSOR_SUBPOS_NOTE) from_x += 2 * XT_RENDER_CELL_W_PIXELS;

	int16_t from_y = (p->select.from_row * XT_RENDER_CELL_H_PIXELS) - xt_phrase_editor_get_cam_y(p);
	int16_t to_y = (p->row * XT_RENDER_CELL_H_PIXELS) - xt_phrase_editor_get_cam_y(p);

	int16_t lesser_x = (from_x <= to_x) ? from_x : to_x;
	int16_t lesser_y = (from_y <= to_y) ? from_y : to_y;
	int16_t greater_x = (from_x > to_x) ? from_x : to_x;
	int16_t greater_y = (from_y > to_y) ? from_y : to_y;

	if (lesser_x < 0) lesser_x = 0;
	if (lesser_y < 0) lesser_y = 0;
	if (greater_x >= (XT_RENDER_NT_CHARS * XT_RENDER_CELL_W_PIXELS)) greater_x = (XT_RENDER_NT_CHARS * XT_RENDER_CELL_W_PIXELS);
	if (greater_y >= (XT_RENDER_NT_CHARS * XT_RENDER_CELL_H_PIXELS)) greater_y = (XT_RENDER_NT_CHARS * XT_RENDER_CELL_H_PIXELS);

	const int16_t sel_w = 1 + (greater_x - lesser_x) / XT_RENDER_CELL_W_PIXELS;
	const int16_t sel_h = 1 + (greater_y - lesser_y) / XT_RENDER_CELL_H_PIXELS;
	xt_cursor_set(lesser_x, lesser_y, sel_w, sel_h, -1, false);

}

// ============================================================================
// Note entry.
// ============================================================================
typedef struct XtKeyNotePairing
{
	XtKeyName key;
	XtNote note;
	int16_t octave_offset;
} XtKeyNotePairing;

static const XtKeyNotePairing note_lookup[] =
{
	{XT_KEY_SPACE, XT_NOTE_NONE, 0},
	{XT_KEY_DEL, XT_NOTE_NONE, 0},

	{XT_KEY_Z, XT_NOTE_C, 0},
	{XT_KEY_S, XT_NOTE_CS, 0},
	{XT_KEY_X, XT_NOTE_D, 0},
	{XT_KEY_D, XT_NOTE_DS, 0},
	{XT_KEY_C, XT_NOTE_E, 0},
	{XT_KEY_V, XT_NOTE_F, 0},
	{XT_KEY_G, XT_NOTE_FS, 0},
	{XT_KEY_B, XT_NOTE_G, 0},
	{XT_KEY_H, XT_NOTE_GS, 0},
	{XT_KEY_N, XT_NOTE_A, 0},
	{XT_KEY_J, XT_NOTE_AS, 0},
	{XT_KEY_M, XT_NOTE_B, 0},
	{XT_KEY_COMMA, XT_NOTE_C, 1},
	{XT_KEY_L, XT_NOTE_CS, 1},
	{XT_KEY_PERIOD, XT_NOTE_CS, 1},

	{XT_KEY_Q, XT_NOTE_C, 1},
	{XT_KEY_2, XT_NOTE_CS, 1},
	{XT_KEY_W, XT_NOTE_D, 1},
	{XT_KEY_3, XT_NOTE_DS, 1},
	{XT_KEY_E, XT_NOTE_E, 1},
	{XT_KEY_R, XT_NOTE_F, 1},
	{XT_KEY_5, XT_NOTE_FS, 1},
	{XT_KEY_T, XT_NOTE_G, 1},
	{XT_KEY_6, XT_NOTE_GS, 1},
	{XT_KEY_Y, XT_NOTE_A, 1},
	{XT_KEY_7, XT_NOTE_AS, 1},
	{XT_KEY_U, XT_NOTE_B, 1},

	{XT_KEY_I, XT_NOTE_C, 2},
	{XT_KEY_9, XT_NOTE_CS, 2},
	{XT_KEY_O, XT_NOTE_D, 2},
	{XT_KEY_0, XT_NOTE_DS, 2},
	{XT_KEY_P, XT_NOTE_E, 2},

	{XT_KEY_1, XT_NOTE_OFF, 0},
	{XT_KEY_A, XT_NOTE_CUT, 0},
};

// Keys to perform entry on the note column. Returns true if a note was entered.
static inline bool handle_note_entry(XtPhraseEditor *p, XtTrack *t,
                                     XtKeyEvent e)
{
	if (e.modifiers & XT_KEY_MOD_SHIFT) return false;
	// TODO: Entry for PCM channels.
	if (p->column >= 8) return false;

	for (uint16_t i = 0; i < ARRAYSIZE(note_lookup); i++)
	{
		const XtKeyNotePairing *m = &note_lookup[i];
		if (e.name == m->key)
		{
			XtPhrase *phrase = xt_track_get_phrase(t, p->column, p->frame);
			XtCell *cell = &phrase->cells[p->row];
			int8_t octave = p->octave + m->octave_offset;
			if (octave < 0) octave = 0;
			else if (octave > 7) octave = 7;
			if (m->note >= XT_NOTE_CUT || m->note == XT_NOTE_NONE) octave = 0;
			cell->note = m->note | (octave << 4);
			cell->inst = p->instrument;
			return true;
		}
	}
	return false;
}

// ============================================================================
// Numerical entry.
// ============================================================================
typedef struct XtKeyNumberPairing
{
	XtKeyName key;
	uint8_t value;
} XtKeyNumberPairing;

static const XtKeyNumberPairing number_lookup[] =
{
	{XT_KEY_0, 0},
	{XT_KEY_1, 1},
	{XT_KEY_2, 2},
	{XT_KEY_3, 3},
	{XT_KEY_4, 4},
	{XT_KEY_5, 5},
	{XT_KEY_6, 6},
	{XT_KEY_7, 7},
	{XT_KEY_8, 8},
	{XT_KEY_9, 9},
	{XT_KEY_A, 0xA},
	{XT_KEY_B, 0xB},
	{XT_KEY_C, 0xC},
	{XT_KEY_D, 0xD},
	{XT_KEY_E, 0xE},
	{XT_KEY_F, 0xF},
	{XT_KEY_NUMPAD_0, 0},
	{XT_KEY_NUMPAD_1, 1},
	{XT_KEY_NUMPAD_2, 2},
	{XT_KEY_NUMPAD_3, 3},
	{XT_KEY_NUMPAD_4, 4},
	{XT_KEY_NUMPAD_5, 5},
	{XT_KEY_NUMPAD_6, 6},
	{XT_KEY_NUMPAD_7, 7},
	{XT_KEY_NUMPAD_8, 8},
	{XT_KEY_NUMPAD_9, 9},
};

static inline bool handle_number_entry(XtPhraseEditor *p, XtTrack *t,
                                           XtKeyEvent e)
{
	for (uint16_t i = 0; i < ARRAYSIZE(number_lookup); i++)
	{
		const XtKeyNumberPairing *m = &number_lookup[i];
		if (e.name == m->key)
		{
			XtPhrase *phrase = xt_track_get_phrase(t, p->column, p->frame);
			XtCell *cell = &phrase->cells[p->row];
			switch (p->sub_pos)
			{
				default:
					return false;
				case CURSOR_SUBPOS_INSTRUMENT_HIGH:
					cell->inst &= 0x0F;
					cell->inst |= m->value << 4;
					return true;
				case CURSOR_SUBPOS_INSTRUMENT_LOW:
					cell->inst &= 0xF0;
					cell->inst |= m->value;
					return true;
				// TODO: Make this automagically handle a variable number of
				// command columns.
				case CURSOR_SUBPOS_ARG1_HIGH:
					cell->cmd[0].arg &= 0x0F;
					cell->cmd[0].arg |= m->value << 4;
					return true;
				case CURSOR_SUBPOS_ARG1_LOW:
					cell->cmd[0].arg &= 0xF0;
					cell->cmd[0].arg |= m->value;
					return true;
			}
		}
	}
	return false;
}

// ============================================================================
// Command entry.
// ============================================================================
typedef struct XtKeyCommandPairing
{
	XtKeyName key;
	XtCmd value;
} XtKeyCommandPairing;

static const XtKeyCommandPairing command_lookup[] =
{
	{XT_KEY_DEL, XT_CMD_NONE},

	{XT_KEY_0, XT_CMD_TL_OP0},
	{XT_KEY_1, XT_CMD_TL_OP1},
	{XT_KEY_2, XT_CMD_TL_OP2},
	{XT_KEY_3, XT_CMD_TL_OP3},

	{XT_KEY_4, XT_CMD_MULT_OP0},
	{XT_KEY_5, XT_CMD_MULT_OP1},
	{XT_KEY_6, XT_CMD_MULT_OP2},
	{XT_KEY_7, XT_CMD_MULT_OP3},

	{XT_KEY_A, XT_CMD_AMPLITUDE},

	{XT_KEY_B, XT_CMD_BREAK},
	{XT_KEY_C, XT_CMD_HALT},
	{XT_KEY_D, XT_CMD_SKIP},

	{XT_KEY_F, XT_CMD_SPEED},

	{XT_KEY_N, XT_CMD_NOISE_EN},

	{XT_KEY_O, XT_CMD_PAN},

	{XT_KEY_P, XT_CMD_PORTAMENTO},

	{XT_KEY_T, XT_CMD_TREMOLO},
	{XT_KEY_V, XT_CMD_VIBRATO},
	{XT_KEY_G, XT_CMD_TREMOLO_TYPE},
	{XT_KEY_H, XT_CMD_VIBRATO_TYPE},

	{XT_KEY_Q, XT_CMD_SLIDE_UP},
	{XT_KEY_R, XT_CMD_SLIDE_DOWN},
	{XT_KEY_S, XT_CMD_MUTE_DELAY},
	{XT_KEY_W, XT_CMD_NOTE_DELAY},
	{XT_KEY_X, XT_CMD_CUT_DELAY},
	{XT_KEY_Z, XT_CMD_TUNE},
};

static inline bool handle_command_entry(XtPhraseEditor *p, XtTrack *t,
                                            XtKeyEvent e)
{
	for (uint16_t i = 0; i < ARRAYSIZE(command_lookup); i++)
	{
		const XtKeyCommandPairing *m = &command_lookup[i];
		if (e.name == m->key)
		{
			XtPhrase *phrase = xt_track_get_phrase(t, p->column, p->frame);
			XtCell *cell = &phrase->cells[p->row];
			switch (p->sub_pos)
			{
				default:
					return false;
				case CURSOR_SUBPOS_CMD1:
					cell->cmd[0].cmd = m->value;
					return true;
			}
		}
	}
	return false;
}

// ============================================================================
// Select logic.
// ============================================================================

static void enter_select(XtPhraseEditor *p)
{
	p->select.from_row = p->row;
	p->select.from_column = p->column;
	p->select.from_sub_pos = p->sub_pos;

	p->state = EDITOR_SELECTING;
}

// ============================================================================
// Event handling.
// ============================================================================


static void on_key_set_mode(XtPhraseEditor *p, XtTrack *t, XtKeyEvent e)
{
	switch (e.name)
	{
		case XT_KEY_DOWN:
		case XT_KEY_UP:
		case XT_KEY_LEFT:
		case XT_KEY_RIGHT:
			if (e.modifiers & XT_KEY_MOD_SHIFT)
			{
				if (p->state != EDITOR_SELECTING)
				{
					enter_select(p);
				}
			}
			else
			{
				p->state = EDITOR_NORMAL;
			}
			break;
		case XT_KEY_ESC:
			p->state = EDITOR_NORMAL;
			break;
		default:
			break;
	}
}

static void normal_on_key(XtPhraseEditor *p, XtTrack *t, XtKeyEvent e)
{
	switch (e.name)
	{
		// Navigation keys.
		case XT_KEY_DOWN:
			cursor_down(p, t, true);
			break;
		case XT_KEY_UP:
			cursor_up(p, t, true);
			break;
		case XT_KEY_RIGHT:
			if (e.modifiers & XT_KEY_MOD_CTRL)
			{
				cursor_column_right(p, t, true);
			}
			else
			{
				cursor_right(p, t, true);
			}
			break;
		case XT_KEY_LEFT:
			if (e.modifiers & XT_KEY_MOD_CTRL)
			{
				cursor_column_left(p, t, true);
			}
			else
			{
				cursor_left(p, t, true);
			}
			break;
		case XT_KEY_R_UP:
			frame_up(p, t);
			break;
		case XT_KEY_R_DOWN:
			frame_down(p, t);
			break;
		case XT_KEY_HOME:
			p->row = 0;
			break;
		// Editor settings.
		case XT_KEY_SEMICOLON:
			if (p->octave > 0) p->octave--;
			break;
		case XT_KEY_COLON:
			if (p->octave < 7) p->octave++;
			break;
		case XT_KEY_NUMPAD_PLUS:
			if (p->instrument < t->num_instruments - 1) p->instrument++;
			break;
		case XT_KEY_NUMPAD_MINUS:
			if (p->instrument > 0) p->instrument--;
			break;
		default:
			break;
	}
	// Entry.
	switch (p->sub_pos)
	{
		case CURSOR_SUBPOS_NOTE:
			if (handle_note_entry(p, t, e))
			{
				p->channel_dirty[p->column] = true;
				cursor_down(p, t, true);
			}
			break;
		case CURSOR_SUBPOS_INSTRUMENT_HIGH:
		case CURSOR_SUBPOS_ARG1_HIGH:
			if (handle_number_entry(p, t, e))
			{
				p->channel_dirty[p->column] = true;
				cursor_right(p, t, true);
			}
			break;
		case CURSOR_SUBPOS_INSTRUMENT_LOW:
		case CURSOR_SUBPOS_ARG1_LOW:
			if (handle_number_entry(p, t, e))
			{
				p->channel_dirty[p->column] = true;
				cursor_down(p, t, true);
				cursor_left(p, t, true);
			}
			break;
		case CURSOR_SUBPOS_CMD1:
			if (handle_command_entry(p, t, e))
			{
				p->channel_dirty[p->column] = true;
				cursor_right(p, t, true);
			}
			break;
		default:
			break;
	}

	cursor_update_cam_column(p);
	draw_normal_cursor(p);
}

static void selecting_on_key(XtPhraseEditor *p, XtTrack *t, XtKeyEvent e)
{
	switch (e.name)
	{
		// Navigation keys.
		case XT_KEY_DOWN:
			cursor_down(p, t, false);
			break;
		case XT_KEY_UP:
			cursor_up(p, t, false);
			break;
		case XT_KEY_RIGHT:
			if (e.modifiers & XT_KEY_MOD_CTRL)
			{
				cursor_column_right(p, t, false);
			}
			else
			{
				cursor_right(p, t, false);
			}
			break;
		case XT_KEY_LEFT:
			if (e.modifiers & XT_KEY_MOD_CTRL)
			{
				cursor_column_left(p, t, false);
			}
			else
			{
				cursor_left(p, t, false);
			}
			break;
		default:
			break;
	}

	cursor_update_cam_column(p);
	draw_select_region(p);
}

void xt_phrase_editor_on_key(XtPhraseEditor *p, XtTrack *t, XtKeyEvent e)
{
	on_key_set_mode(p, t, e);

	switch (p->state)
	{
		case EDITOR_NORMAL:
			normal_on_key(p, t, e);
			break;
		case EDITOR_SELECTING:
			selecting_on_key(p, t, e);
			break;
		default:
			break;
	}
}
