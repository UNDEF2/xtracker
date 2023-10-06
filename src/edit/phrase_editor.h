#ifndef EDIT_PHRASE_EDITOR_H
#define EDIT_PHRASE_EDITOR_H

#include "xt/track.h"
#include "ui/track_render.h"
#include "xbase/keys.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum XtEditorCursorSubPos
{
	CURSOR_SUBPOS_NOTE,
	CURSOR_SUBPOS_INSTRUMENT_HIGH,
	CURSOR_SUBPOS_INSTRUMENT_LOW,
	CURSOR_SUBPOS_VOL_HIGH,
	CURSOR_SUBPOS_VOL_LOW,
	CURSOR_SUBPOS_CMD1,
	CURSOR_SUBPOS_ARG1_HIGH,
	CURSOR_SUBPOS_ARG1_LOW,
	CURSOR_SUBPOS_CMD2,
	CURSOR_SUBPOS_ARG2_HIGH,
	CURSOR_SUBPOS_ARG2_LOW,
	CURSOR_SUBPOS_CMD3,
	CURSOR_SUBPOS_ARG3_HIGH,
	CURSOR_SUBPOS_ARG3_LOW,
	CURSOR_SUBPOS_MAX_INVALID,

	CURSOR_SUBPOS_INVALID = 0xFFFF,
} XtEditorCursorSubPos;

typedef enum XtEditorState
{
	EDITOR_NORMAL,
	EDITOR_SELECTING,
} XtEditorState;

typedef struct XtPhraseEditor
{
	XtEditorState state;
	XtEditorCursorSubPos sub_pos;  // What is being edited within the column.
	int16_t frame;  // Which frame is being edited.
	int16_t row;  // Index into the Frame.
	int16_t column;  // Horizontally, which column the cursor lies in.

	int16_t instrument;  // Currently selected instrument number.
	uint16_t octave;  // Currently selected entry octave (for the bottom row).
	uint16_t step_size;  // Rows to go down after having entered a note.

	bool channel_dirty[XT_TOTAL_CHANNEL_COUNT];

	struct
	{
		int16_t ctrl_a_step;
		int16_t from_row;
		int16_t from_column;
		XtEditorCursorSubPos from_sub_pos;
	} select;

	// Buffer for copy/paste, spanning the size of one screen.
	struct
	{
		XtPhrase phrase_buffer[XT_TOTAL_CHANNEL_COUNT];
		XtEditorCursorSubPos left_sub_pos;
		XtEditorCursorSubPos right_sub_pos;
		int16_t rows;
		int16_t columns;
	} copy;

	// Left-most column index on-screen. This is "pushed" when the cursor is
	// >= 5 columns away.
	int16_t cam_column;

	int16_t visible_channels;
	int16_t total_channels;
} XtPhraseEditor;

void xt_phrase_editor_init(XtPhraseEditor *p, const XtTrack *t);

void xt_phrase_editor_on_key(XtPhraseEditor *p, XtTrack *t, XBKeyEvent e);

// Mark channel(s) as dirty in the XtTrackRenderer
void xt_phrase_editor_update_renderer(XtPhraseEditor *p, XtTrackRenderer *r);
void xt_phrase_editor_set_camera(XtPhraseEditor *p, XtTrackRenderer *r);

// Goes to standard (non-selecting) context and repaints the cursor.
void xt_phrase_editor_on_focus_acquired(XtPhraseEditor *p);

int16_t xt_phrase_editor_get_cam_x(const XtPhraseEditor *p);
int16_t xt_phrase_editor_get_cam_y(const XtPhraseEditor *p);

void xt_phrase_editor_set_fnlabels(bool ctrl);

#endif  // EDIT_PHRASE_EDITOR_H
