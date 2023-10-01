#ifndef UI_INSTRUMENT_RENDER_H
#define UI_INSTRUMENT_RENDER_H

#include "xt_track.h"
#include "xt_instrument.h"
#include "phrase_editor.h"

#include <stdbool.h>

typedef struct XtInstrumentRenderer
{
	// Data for currently drawn instrument.
	XtInstrument data;
	int16_t instr_num;

	// Edit interface vars
	struct
	{
		int16_t row, column;  // Selection pos
	} edit;

	bool backing_drawn;
} XtInstrumentRenderer;

// Set up the XtInstrument struct for its first render and further use.
void xt_instrument_renderer_init(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p);

// Draw the instrumentment table as-needed based on track data and the provided
// navigation position. Pass -1 to the row or column parameters to not update
// positioning.
void xt_instrument_renderer_tick(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p, int16_t instr_num);

// Mark all frames and the border as needing a redraw.
void xt_instrument_renderer_request_redraw(XtInstrumentRenderer *a);


#endif  // UI_INSTRUMENT_RENDER_H
