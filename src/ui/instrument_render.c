#include "ui/instrument_render.h"
#include "ui/metrics.h"
#include "xt_instrument.h"
#include <string.h>
#include "util/cgprint.h"
#include "xt_palette.h"
#include "common.h"

void xt_instrument_renderer_init(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p)
{
	memset(a, 0, sizeof(*a));
	a->instr_num = -1;
}

// Draw the instrumentment table as-needed based on track data and the provided
// navigation position.
void xt_instrument_renderer_tick(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p, int16_t instr_num)
{
	if (!a->backing_drawn)
	{


		a->backing_drawn = true;
	}

	if (a->instr_num != instr_num)
	{
		a->instr_num = instr_num;
	}
}

// Mark all frames and the border as needing a redraw.
void xt_instrument_renderer_request_redraw(XtInstrumentRenderer *a)
{
	a->instr_num = -1;
	a->backing_drawn = false;
}
