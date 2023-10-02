#ifndef UI_REGDATA_RENDER_H
#define UI_REGDATA_RENDER_H

#include "xt_track.h"
#include "xt_instrument.h"
#include "phrase_editor.h"

#include <stdbool.h>

typedef struct XtRegdataRenderer
{
	// Data for currently drawn instrument.
	XtInstrument data;

	// Edit interface vars
	struct
	{
		int16_t row, column;  // Selection pos
	} edit;

	bool full_repaint;
	bool data_repaint;
} XtRegdataRenderer;

// Set up the XtInstrument struct for its first render and further use.
void xt_regdata_renderer_init(XtRegdataRenderer *a, XtTrack *t, XtPhraseEditor *p);

// Draw the instrumentment table as-needed based on track data and the provided
// navigation position. Pass -1 to the row or column parameters to not update
// positioning.
void xt_regdata_renderer_tick(XtRegdataRenderer *a, XtTrack *t, XtPhraseEditor *p, int16_t instr_num);

// Mark all frames and the border as needing a redraw.
void xt_regdata_renderer_request_redraw(XtRegdataRenderer *a, bool content_only);


#endif  // UI_REGDATA_RENDER_H
