#include "phrase_editor.h"
#include "xbase/keys.h"
#include "ui/cursor.h"
#include "common.h"

#include "ui/fnlabels.h"
#include "util/transpose.h"

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
	XBKey key;
	XtNote note;
	int16_t octave_offset;
} XtKeyNotePairing;

static const XtKeyNotePairing note_lookup[] =
{
	{XB_KEY_SPACE, XT_NOTE_NONE, 0},
	{XB_KEY_DEL, XT_NOTE_NONE, 0},

	{XB_KEY_Z, XT_NOTE_C, 0},
	{XB_KEY_S, XT_NOTE_CS, 0},
	{XB_KEY_X, XT_NOTE_D, 0},
	{XB_KEY_D, XT_NOTE_DS, 0},
	{XB_KEY_C, XT_NOTE_E, 0},
	{XB_KEY_V, XT_NOTE_F, 0},
	{XB_KEY_G, XT_NOTE_FS, 0},
	{XB_KEY_B, XT_NOTE_G, 0},
	{XB_KEY_H, XT_NOTE_GS, 0},
	{XB_KEY_N, XT_NOTE_A, 0},
	{XB_KEY_J, XT_NOTE_AS, 0},
	{XB_KEY_M, XT_NOTE_B, 0},
	{XB_KEY_COMMA, XT_NOTE_C, 1},
	{XB_KEY_L, XT_NOTE_CS, 1},
	{XB_KEY_PERIOD, XT_NOTE_CS, 1},

	{XB_KEY_Q, XT_NOTE_C, 1},
	{XB_KEY_2, XT_NOTE_CS, 1},
	{XB_KEY_W, XT_NOTE_D, 1},
	{XB_KEY_3, XT_NOTE_DS, 1},
	{XB_KEY_E, XT_NOTE_E, 1},
	{XB_KEY_R, XT_NOTE_F, 1},
	{XB_KEY_5, XT_NOTE_FS, 1},
	{XB_KEY_T, XT_NOTE_G, 1},
	{XB_KEY_6, XT_NOTE_GS, 1},
	{XB_KEY_Y, XT_NOTE_A, 1},
	{XB_KEY_7, XT_NOTE_AS, 1},
	{XB_KEY_U, XT_NOTE_B, 1},

	{XB_KEY_I, XT_NOTE_C, 2},
	{XB_KEY_9, XT_NOTE_CS, 2},
	{XB_KEY_O, XT_NOTE_D, 2},
	{XB_KEY_0, XT_NOTE_DS, 2},
	{XB_KEY_P, XT_NOTE_E, 2},

	{XB_KEY_1, XT_NOTE_OFF, 0},
	{XB_KEY_A, XT_NOTE_CUT, 0},
};

// Keys to perform entry on the note column. Returns true if a note was entered.
static inline bool handle_note_entry(XtPhraseEditor *p, XtTrack *t,
                                     XBKeyEvent e)
{
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
	XBKey key;
	uint8_t value;
} XtKeyNumberPairing;

static const XtKeyNumberPairing number_lookup[] =
{
	{XB_KEY_0, 0},
	{XB_KEY_1, 1},
	{XB_KEY_2, 2},
	{XB_KEY_3, 3},
	{XB_KEY_4, 4},
	{XB_KEY_5, 5},
	{XB_KEY_6, 6},
	{XB_KEY_7, 7},
	{XB_KEY_8, 8},
	{XB_KEY_9, 9},
	{XB_KEY_A, 0xA},
	{XB_KEY_B, 0xB},
	{XB_KEY_C, 0xC},
	{XB_KEY_D, 0xD},
	{XB_KEY_E, 0xE},
	{XB_KEY_F, 0xF},
	{XB_KEY_NUMPAD_0, 0},
	{XB_KEY_NUMPAD_1, 1},
	{XB_KEY_NUMPAD_2, 2},
	{XB_KEY_NUMPAD_3, 3},
	{XB_KEY_NUMPAD_4, 4},
	{XB_KEY_NUMPAD_5, 5},
	{XB_KEY_NUMPAD_6, 6},
	{XB_KEY_NUMPAD_7, 7},
	{XB_KEY_NUMPAD_8, 8},
	{XB_KEY_NUMPAD_9, 9},
};

static inline bool handle_number_entry(XtPhraseEditor *p, XtTrack *t,
                                           XBKeyEvent e)
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
	XBKey key;
	XtCmd value;
} XtKeyCommandPairing;

static const XtKeyCommandPairing command_lookup[] =
{
	{XB_KEY_DEL, XT_CMD_NONE},

	{XB_KEY_0, XT_CMD_TL_OP0},
	{XB_KEY_1, XT_CMD_TL_OP1},
	{XB_KEY_2, XT_CMD_TL_OP2},
	{XB_KEY_3, XT_CMD_TL_OP3},

	{XB_KEY_4, XT_CMD_MULT_OP0},
	{XB_KEY_5, XT_CMD_MULT_OP1},
	{XB_KEY_6, XT_CMD_MULT_OP2},
	{XB_KEY_7, XT_CMD_MULT_OP3},

	{XB_KEY_A, XT_CMD_AMPLITUDE},

	{XB_KEY_B, XT_CMD_BREAK},
	{XB_KEY_C, XT_CMD_HALT},
	{XB_KEY_D, XT_CMD_SKIP},

	{XB_KEY_F, XT_CMD_SPEED},

	{XB_KEY_N, XT_CMD_NOISE_EN},

	{XB_KEY_O, XT_CMD_PAN},

	{XB_KEY_T, XT_CMD_TREMOLO},
	{XB_KEY_V, XT_CMD_VIBRATO},
	{XB_KEY_G, XT_CMD_TREMOLO_TYPE},
	{XB_KEY_H, XT_CMD_VIBRATO_TYPE},

	{XB_KEY_Q, XT_CMD_SLIDE_UP},
	{XB_KEY_R, XT_CMD_SLIDE_DOWN},
	{XB_KEY_S, XT_CMD_MUTE_DELAY},
	{XB_KEY_W, XT_CMD_NOTE_DELAY},
	{XB_KEY_X, XT_CMD_CUT_DELAY},
	{XB_KEY_Z, XT_CMD_TUNE},
};

static inline bool handle_command_entry(XtPhraseEditor *p, XtTrack *t,
                                            XBKeyEvent e)
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
	p->select.ctrl_a_step = 0;

	p->state = EDITOR_SELECTING;
}

// Sets the select rectangle coordinates and row/col counts.
static void select_region_sub(XtPhraseEditor *p,
                              uint16_t *lesser_row, uint16_t *greater_row,
                              uint16_t *row_count,
                              uint16_t *lesser_column, uint16_t *greater_column,
                              uint16_t *column_count)
{
	const uint16_t from_row = p->select.from_row;
	const uint16_t to_row = p->row;
	if (to_row >= from_row)
	{
		*lesser_row = from_row;
		*greater_row = to_row;
	}
	else
	{
		*lesser_row = to_row;
		*greater_row = from_row;
	}
	*row_count = 1 + *greater_row - *lesser_row;

	const uint16_t from_column = p->select.from_column;
	const uint16_t to_column = p->column;
	if (to_column >= from_column)
	{
		*lesser_column = from_column;
		*greater_column = to_column;
	}
	else
	{
		*lesser_column = to_column;
		*greater_column = from_column;
	}
	*column_count = 1 + *greater_column - *lesser_column;
}

// Sets the left and right subposition ranges based upon selection dimensions.
static void select_subpos_sub(XtPhraseEditor *p,
                              XtEditorCursorSubPos *left_sub_pos,
                              XtEditorCursorSubPos *right_sub_pos)
{
	if (p->select.from_column < p->column)
	{
		*left_sub_pos = p->select.from_sub_pos;
		*right_sub_pos = p->sub_pos;
	}
	else if (p->select.from_column > p->column)
	{
		*left_sub_pos = p->sub_pos;
		*right_sub_pos = p->select.from_sub_pos;
	}
	else
	{
		*left_sub_pos = (p->sub_pos < p->select.from_sub_pos) ? p->sub_pos : p->select.from_sub_pos;
		*right_sub_pos = (p->sub_pos < p->select.from_sub_pos) ? p->select.from_sub_pos : p->sub_pos;
	}
}

// Based upon which column is being checked, the range of subpositions to be
// iterated through is set.
static void select_limit_subpos_by_col_sub(XtPhraseEditor *p,
                                           uint16_t column, uint16_t column_count,
                                           XtEditorCursorSubPos base_left_sub_pos,
                                           XtEditorCursorSubPos base_right_sub_pos,
                                           XtEditorCursorSubPos *new_left_sub_pos,
                                           XtEditorCursorSubPos *new_right_sub_pos)
{
	const bool left_edge = (column == 0);
	const bool right_edge = (column == column_count - 1);

	*new_left_sub_pos = left_edge ? base_left_sub_pos : CURSOR_SUBPOS_NOTE;
	*new_right_sub_pos = right_edge ? base_right_sub_pos : CURSOR_SUBPOS_ARG1_LOW;
}

// Performs a copy from the selected region.
static void select_copy(XtPhraseEditor *p, XtTrack *t)
{
	uint16_t lesser_row, greater_row, row_count;
	uint16_t lesser_column, greater_column, column_count;
	select_region_sub(p, &lesser_row, &greater_row, &row_count,
	                  &lesser_column, &greater_column, &column_count);
	p->copy.rows = row_count;
	p->copy.columns = column_count;

	// copy takes all data from relevant columns, without regard for sub position.
	for (uint16_t column = 0; column < column_count; column++)
	{
		const XtPhrase *src_phrase = xt_track_get_phrase(t, column + lesser_column, p->frame);
		XtPhrase *dest_phrase = &p->copy.phrase_buffer[column];
		for (uint16_t row = 0; row < row_count; row++)
		{
			const XtCell *src_cell = &src_phrase->cells[row + lesser_row];
			XtCell *dest_cell = &dest_phrase->cells[row];
			*dest_cell = *src_cell;
		}
	}

	select_subpos_sub(p, &p->copy.left_sub_pos, &p->copy.right_sub_pos);
}

// Deletes data within the selected region, with respect to subpos.
static void select_delete(XtPhraseEditor *p, XtTrack *t)
{
	uint16_t lesser_row, greater_row, row_count;
	uint16_t lesser_column, greater_column, column_count;
	select_region_sub(p, &lesser_row, &greater_row, &row_count,
	                  &lesser_column, &greater_column, &column_count);

	XtEditorCursorSubPos sel_left_sub_pos, sel_right_sub_pos;
	select_subpos_sub(p, &sel_left_sub_pos, &sel_right_sub_pos);

	for (uint16_t column = 0; column < column_count; column++)
	{
		const uint16_t col_idx = lesser_column + column;
		if (col_idx >= XT_TOTAL_CHANNEL_COUNT) continue;
		XtPhrase *dest_phrase = xt_track_get_phrase(t, col_idx, p->frame);
		for (uint16_t row = 0; row < row_count; row++)
		{
			const uint16_t row_idx = lesser_row + row;
			if (row_idx >= XT_PHRASE_MAX_ROWS) continue;
			XtCell *dest_cell = &dest_phrase->cells[row_idx];

			XtEditorCursorSubPos left_sub_pos;
			XtEditorCursorSubPos right_sub_pos;
			select_limit_subpos_by_col_sub(p, column, column_count,
			                               sel_left_sub_pos, sel_right_sub_pos,
			                               &left_sub_pos, &right_sub_pos);

			for (XtEditorCursorSubPos sub = left_sub_pos; sub < right_sub_pos + 1; sub++)
			{
				switch (sub)
				{
					case CURSOR_SUBPOS_NOTE:
						dest_cell->note = XT_NOTE_NONE;
						break;
					case CURSOR_SUBPOS_INSTRUMENT_HIGH:
						dest_cell->inst &= 0x0F;
						break;
					case CURSOR_SUBPOS_INSTRUMENT_LOW:
						dest_cell->inst &= 0xF0;
						break;
					case CURSOR_SUBPOS_CMD1:
						dest_cell->cmd[0].cmd = XT_CMD_NONE;
						break;
					case CURSOR_SUBPOS_ARG1_HIGH:
						dest_cell->cmd[0].arg &= 0x0F;
						break;
					case CURSOR_SUBPOS_ARG1_LOW:
						dest_cell->cmd[0].arg &= 0xF0;
						break;
					default:
						break;
				}
			}
		}

		p->channel_dirty[col_idx] = true;
	}
}

// Pastes data at the cursor position, with respect to subpos.
static void paste(XtPhraseEditor *p, XtTrack *t)
{
	const uint16_t row_count = p->copy.rows;
	const uint16_t column_count = p->copy.columns;

	for (uint16_t column = 0; column < column_count; column++)
	{
		const uint16_t col_idx = p->column + column;
		if (col_idx >= XT_TOTAL_CHANNEL_COUNT) continue;
		const XtPhrase *src_phrase = &p->copy.phrase_buffer[column];
		XtPhrase *dest_phrase = xt_track_get_phrase(t, col_idx, p->frame);

		for (uint16_t row = 0; row < row_count; row++)
		{
			const uint16_t row_idx = p->row + row;
			if (row_idx >= XT_PHRASE_MAX_ROWS) continue;
			const XtCell *src_cell = &src_phrase->cells[row];
			XtCell *dest_cell = &dest_phrase->cells[row_idx];

			XtEditorCursorSubPos left_sub_pos;
			XtEditorCursorSubPos right_sub_pos;
			select_limit_subpos_by_col_sub(p, column, column_count,
			                               p->copy.left_sub_pos, p->copy.right_sub_pos,
			                               &left_sub_pos, &right_sub_pos);

			for (XtEditorCursorSubPos sub = left_sub_pos; sub < right_sub_pos + 1; sub++)
			{
				switch (sub)
				{
					case CURSOR_SUBPOS_NOTE:
						dest_cell->note = src_cell->note;
						break;
					case CURSOR_SUBPOS_INSTRUMENT_HIGH:
						dest_cell->inst &= 0x0F;
						dest_cell->inst |= src_cell->inst & 0xF0;
						break;
					case CURSOR_SUBPOS_INSTRUMENT_LOW:
						dest_cell->inst &= 0xF0;
						dest_cell->inst |= src_cell->inst & 0x0F;
						break;
					case CURSOR_SUBPOS_CMD1:
						dest_cell->cmd[0].cmd = src_cell->cmd[0].cmd;
						break;
					case CURSOR_SUBPOS_ARG1_HIGH:
						dest_cell->cmd[0].arg &= 0x0F;
						dest_cell->cmd[0].arg |= src_cell->cmd[0].arg & 0xF0;
						break;
					case CURSOR_SUBPOS_ARG1_LOW:
						dest_cell->cmd[0].arg &= 0xF0;
						dest_cell->cmd[0].arg |= src_cell->cmd[0].arg & 0x0F;
						break;
					default:
						break;
				}
			}
		}

		p->channel_dirty[col_idx] = true;
		dest_phrase->phrase_valid = true;
	}
}

// ============================================================================
// Note Manipulation Functions.
// ============================================================================

static void transpose(XtPhraseEditor *p, XtTrack *t,
                      int16_t semitones, bool use_selection)
{
	if (!use_selection)
	{
		XtPhrase *phrase = xt_track_get_phrase(t, p->column, p->frame);
		XtCell *cell = &phrase->cells[p->row];
		cell->note = xt_transpose_note(cell->note, semitones);
		p->channel_dirty[p->column] = true;
		return;
	}

	uint16_t lesser_row, greater_row, row_count;
	uint16_t lesser_column, greater_column, column_count;
	select_region_sub(p, &lesser_row, &greater_row, &row_count,
	                  &lesser_column, &greater_column, &column_count);

	for (uint16_t column = 0; column < column_count; column++)
	{
		const uint16_t col_idx = lesser_column + column;
		if (col_idx >= XT_TOTAL_CHANNEL_COUNT) continue;
		XtPhrase *dest_phrase = xt_track_get_phrase(t, col_idx, p->frame);
		for (uint16_t row = 0; row < row_count; row++)
		{
			const uint16_t row_idx = lesser_row + row;
			if (row_idx >= XT_PHRASE_MAX_ROWS) continue;
			XtCell *dest_cell = &dest_phrase->cells[row_idx];
			dest_cell->note = xt_transpose_note(dest_cell->note, semitones);
		}

		p->channel_dirty[col_idx] = true;
	}
}

static void shrink(XtPhraseEditor *p, XtTrack *t, bool use_selection)
{
	(void)p;
	(void)t;
	(void)use_selection;
}

static void expand(XtPhraseEditor *p, XtTrack *t, bool use_selection)
{
	uint16_t lesser_row, greater_row, row_count;
	uint16_t lesser_column, greater_column, column_count;
	if (use_selection)
	{
		select_region_sub(p, &lesser_row, &greater_row, &row_count,
		                  &lesser_column, &greater_column, &column_count);
	}
	else
	{
		// If not using the selection region, just apply to the whole phrase.
		lesser_row = 0;
		greater_row = XT_PHRASE_MAX_ROWS - 1;
		row_count = XT_PHRASE_MAX_ROWS;
		lesser_column = p->column;
		greater_column = p->column;
		column_count = 1;
	}
	for (uint16_t column = 0; column < column_count; column++)
	{
		const uint16_t col_idx = lesser_column + column;
		if (col_idx >= XT_TOTAL_CHANNEL_COUNT) continue;
		XtPhrase *dest_phrase = xt_track_get_phrase(t, col_idx, p->frame);
		for (uint16_t row = 0; row < row_count; row++)
		{
			const uint16_t row_idx = (greater_row - row);
			const bool blank = row_idx % 2 != 0;
			if (row_idx >= XT_PHRASE_MAX_ROWS && row_idx < 0) continue;
			XtCell *dest_cell = &dest_phrase->cells[row_idx];
			if (blank)
			{
				*dest_cell = (XtCell){ 0 };
			}
			else
			{
				*dest_cell = dest_phrase->cells[row_idx / 2];
			}
		}

		p->channel_dirty[col_idx] = true;
	}
}

// ============================================================================
// Event handling.
// ============================================================================

static void on_key_set_mode(XtPhraseEditor *p, XtTrack *t, XBKeyEvent e)
{
	switch (e.name)
	{
		case XB_KEY_DOWN:
		case XB_KEY_UP:
		case XB_KEY_LEFT:
		case XB_KEY_RIGHT:
			if (e.modifiers & XB_KEY_MOD_SHIFT)
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

		case XB_KEY_A:
			if (e.modifiers & XB_KEY_MOD_CTRL)
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

		case XB_KEY_ESC:
			p->state = EDITOR_NORMAL;
			break;

		default:
			break;
	}
}

static void normal_on_key(XtPhraseEditor *p, XtTrack *t, XBKeyEvent e)
{
	bool note_entry_ok = true;
	switch (e.name)
	{
		// Navigation.
		case XB_KEY_DOWN:
			cursor_down(p, t, true);
			break;
		case XB_KEY_UP:
			cursor_up(p, t, true);
			break;
		case XB_KEY_RIGHT:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				cursor_column_right(p, t, true);
			}
			else
			{
				cursor_right(p, t, true);
			}
			break;
		case XB_KEY_LEFT:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				cursor_column_left(p, t, true);
			}
			else
			{
				cursor_left(p, t, true);
			}
			break;
		// Frame navigation.
		case XB_KEY_R_UP:
			frame_up(p, t);
			break;
		case XB_KEY_R_DOWN:
			frame_down(p, t);
			break;
		case XB_KEY_HOME:
			p->row = 0;
			break;
		// Editor settings.
		case XB_KEY_SEMICOLON:
			if (p->octave > 0) p->octave--;
			break;
		case XB_KEY_COLON:
			if (p->octave < 7) p->octave++;
			break;
		case XB_KEY_NUMPAD_PLUS:
			if (p->instrument < t->num_instruments - 1) p->instrument++;
			break;
		case XB_KEY_NUMPAD_MINUS:
			if (p->instrument > 0) p->instrument--;
			break;
		// Transposition.
		case XB_KEY_F1:
			transpose(p, t, (e.modifiers & XB_KEY_MOD_CTRL) ? -12 : -1, false);
			break;
		case XB_KEY_F2:
			transpose(p, t, (e.modifiers & XB_KEY_MOD_CTRL) ? +12 : +1, false);
			break;
		// Expand / Shrink.
		case XB_KEY_F3:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				shrink(p, t, false);
			}
			else
			{
				expand(p, t, false);
			}
		// Copy / paste.
		case XB_KEY_V:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				paste(p, t);
				note_entry_ok = false;
			}
			break;
		case XB_KEY_C:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				p->select.from_row = p->row;
				p->select.from_column = p->column;
				p->select.from_sub_pos = p->sub_pos;
				select_copy(p, t);
				note_entry_ok = false;
			}
			break;
		case XB_KEY_X:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				p->select.from_row = p->row;
				p->select.from_column = p->column;
				p->select.from_sub_pos = p->sub_pos;
				select_copy(p, t);
				select_delete(p, t);
				note_entry_ok = false;
			}
			break;
		default:
			break;
	}

	// Note entry.
	if (note_entry_ok)
	{
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
	}

	cursor_update_cam_column(p);
	draw_normal_cursor(p);
}

static void selecting_on_key(XtPhraseEditor *p, XtTrack *t, XBKeyEvent e)
{
	if (e.name != XB_KEY_A) p->select.ctrl_a_step = 0;
	switch (e.name)
	{
		// Selection rectangle modification.
		case XB_KEY_DOWN:
			cursor_down(p, t, false);
			break;
		case XB_KEY_UP:
			cursor_up(p, t, false);
			break;
		case XB_KEY_RIGHT:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				cursor_column_right(p, t, false);
			}
			else
			{
				cursor_right(p, t, false);
			}
			break;
		case XB_KEY_LEFT:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				cursor_column_left(p, t, false);
			}
			else
			{
				cursor_left(p, t, false);
			}
			break;
		// Select All (stepped)
		case XB_KEY_A:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				if (p->select.ctrl_a_step == 0)
				{
					p->sub_pos = CURSOR_SUBPOS_NOTE;
					p->row = 0;
					p->select.from_sub_pos = CURSOR_SUBPOS_ARG1_LOW;
					p->select.from_row = XT_PHRASE_MAX_ROWS - 1;
				}
				else if (p->select.ctrl_a_step == 1)
				{
					p->column = 0;
					p->select.from_column = XT_TOTAL_CHANNEL_COUNT - 1;
				}
				p->select.ctrl_a_step++;
			}
			break;
		// Transposition.
		case XB_KEY_F1:
			transpose(p, t, (e.modifiers & XB_KEY_MOD_CTRL) ? -12 : -1, true);
			break;
		case XB_KEY_F2:
			transpose(p, t, (e.modifiers & XB_KEY_MOD_CTRL) ? +12 : +1, true);
			break;
		// Expand / Shrink.
		case XB_KEY_F3:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				shrink(p, t, true);
			}
			else
			{
				expand(p, t, true);
			}
		// Copy / Paste.
		case XB_KEY_C:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				select_copy(p, t);
			}
			break;
		case XB_KEY_X:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				select_copy(p, t);
				select_delete(p, t);
			}
			break;
		case XB_KEY_DEL:
			select_delete(p, t);
			break;
		case XB_KEY_V:
			if (e.modifiers & XB_KEY_MOD_CTRL)
			{
				paste(p, t);
			}
			break;
		default:
			break;
	}

	cursor_update_cam_column(p);
	draw_select_region(p);
}

void xt_phrase_editor_on_key(XtPhraseEditor *p, XtTrack *t, XBKeyEvent e)
{
	if (e.modifiers & XB_KEY_MOD_KEY_UP) return;
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

void xt_phrase_editor_set_fnlabels(bool ctrl)
{
	if (ctrl)
	{
		ui_fnlabel_set(0, "Note-12");
		ui_fnlabel_set(1, "Note+12");
		ui_fnlabel_set(2, "Shrink");
		ui_fnlabel_set(3, "Pull Up");
		ui_fnlabel_set(4, "SetInst");
	}
	else
	{
		ui_fnlabel_set(0, "Note -1");
		ui_fnlabel_set(1, "Note +1");
		ui_fnlabel_set(2, "Expand");
		ui_fnlabel_set(3, "Push Dn");
		ui_fnlabel_set(4, "SetInst");
	}
}
