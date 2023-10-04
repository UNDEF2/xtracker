#ifndef UI_REGDATA_RENDER_H
#define UI_REGDATA_RENDER_H

#include "xt/track.h"
#include "xt/instrument.h"

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
void xt_regdata_renderer_init(XtRegdataRenderer *a);

void xt_regdata_renderer_tick(XtRegdataRenderer *a, const XtInstrument *ins);

// Mark all frames and the border as needing a redraw.
void xt_regdata_renderer_request_redraw(XtRegdataRenderer *a, bool content_only);


#endif  // UI_REGDATA_RENDER_H
