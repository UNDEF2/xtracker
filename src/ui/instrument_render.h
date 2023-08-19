#ifndef UI_INSTRUMENT_RENDER_H
#define UI_INSTRUMENT_RENDER_H

#include "xt_track.h"
#include "xt_instrument.h"
#include "phrase_editor.h"

#include <stdbool.h>

#define XT_INSTRUMENT_RENDER_X 256
#define XT_INSTRUMENT_RENDER_Y 4

typedef struct XtInstrumentRenderer
{
	// Data for currently drawn instrument.
	XtInstrument data;

	// Selection position.
	int16_t row;
	int16_t column;

	int16_t draw_x, draw_y;

	bool border_drawn;
} XtInstrumentRenderer;

// Set up the XtInstrument struct for its first render and further use.
void xt_instrument_renderer_init(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p);

// Draw the instrumentment table as-needed based on track data and the provided
// navigation position. Pass -1 to the row or column parameters to not update
// positioning.
void xt_instrument_renderer_tick(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p);

// Mark all frames and the border as needing a redraw.
void xt_instrument_renderer_redraw(XtInstrumentRenderer *a);


#endif  // UI_INSTRUMENT_RENDER_H
