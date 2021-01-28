#ifndef _XT_PHRASE_EDITOR_H
#define _XT_PHRASE_EDITOR_H

#include "xt_track.h"
#include "xt_keys.h"
#include "xt_render.h"
#include <stdint.h>

typedef enum XtEditorCursorSubPos
{
	CURSOR_SUBPOS_NOTE,
	CURSOR_SUBPOS_INSTRUMENT_HIGH,
	CURSOR_SUBPOS_INSTRUMENT_LOW,
	CURSOR_SUBPOS_CMD1,
	CURSOR_SUBPOS_ARG1_HIGH,
	CURSOR_SUBPOS_ARG1_LOW,
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
	int16_t octave;  // Currently selected entry octave (for the bottom row).
	int16_t step_size;  // Rows to go down after having entered a note.

	int8_t channel_dirty[XT_TOTAL_CHANNEL_COUNT];
	// TODO: Clipboard buffer, and all that...

} XtPhraseEditor;

void xt_phrase_editor_init(XtPhraseEditor *p);

void xt_phrase_editor_tick(XtPhraseEditor *p, XtTrack *t, const XtKeys *k);

// Mark channel(s) as dirty in the XtTrackRenderer
void xt_phrase_editor_update_renderer(XtPhraseEditor *p, XtTrackRenderer *r);

// Draw the cursor, offset by the current "camera" position.
void xt_phrase_editor_draw_cursor(const XtPhraseEditor *p,
                                  int16_t cam_x, int16_t cam_y);

#endif  // _XT_PHRASE_EDITOR_H
