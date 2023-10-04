#ifndef UI_INSTRUMENT_LIST_RENDER_H
#define UI_INSTRUMENT_LIST_RENDER_H

#include "xt/track.h"
#include "xt/instrument.h"

#include <stdbool.h>

typedef struct XtInstrumentListRenderer
{
	int16_t instrument_id;

	// Edit interface vars
	struct
	{
		int16_t row, column;  // Selection pos
	} edit;

	bool full_repaint;
	bool data_repaint;
} XtInstrumentListRenderer;

// Set up the XtInstrument struct for its first render and further use.
void xt_instrument_list_renderer_init(XtInstrumentListRenderer *a);

void xt_instrument_list_renderer_tick(XtInstrumentListRenderer *a,
                                      const XtTrack *t, int16_t instrument_id);

// Mark all frames and the border as needing a redraw.
void xt_instrument_list_renderer_request_redraw(XtInstrumentListRenderer *a,
                                                bool content_only);


#endif  // UI_INSTRUMENT_LIST_RENDER_H
