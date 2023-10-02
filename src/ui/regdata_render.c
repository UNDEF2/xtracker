#include "ui/regdata_render.h"
#include "ui/metrics.h"
#include "ui/section_title.h"
#include "xt_instrument.h"
#include "util/cgprint.h"
#include "xt_palette.h"
#include "common.h"
#include <string.h>

void xt_regdata_renderer_init(XtRegdataRenderer *a, XtTrack *t, XtPhraseEditor *p)
{
	memset(a, 0, sizeof(*a));
	a->full_repaint = true;
}

// Draw the instrument table as-needed based on track data and the provided
// navigation position.
void xt_regdata_renderer_tick(XtRegdataRenderer *a, XtTrack *t, XtPhraseEditor *p, int16_t instr_num)
{
	const int16_t kbase_x = XT_UI_ARRANGEMENT_X;
	const int16_t kbase_y = XT_UI_ARRANGEMENT_Y;
	const int16_t kbase_w = XT_UI_AREA_W - kbase_x - XT_UI_MARGIN_SIDE;
	const int16_t kbase_h = XT_UI_AREA_H - kbase_y - 8;  // Space for the channel indicators

	// If the current instrument type differs from the last, do a full redraw.
	if (t->instruments[p->instrument].type != a->data.type)
	{
		a->full_repaint = true;
	}

	if (a->full_repaint)
	{
		ui_section_title_draw("Instrument Data (OPM)", kbase_x, kbase_y, kbase_w, kbase_h);
		// As part of a full repaint, the instrument data will be redrawn as well.
		a->full_repaint = false;
		a->data_repaint = true;
	}
}

// Mark all frames and the border as needing a redraw.
void xt_regdata_renderer_request_redraw(XtRegdataRenderer *a, bool content_only)
{
	a->data_repaint = true;
	if (!content_only) a->full_repaint = true;
}
