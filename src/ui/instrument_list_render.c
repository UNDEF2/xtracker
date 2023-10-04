#include "ui/instrument_list_render.h"
#include "ui/metrics.h"
#include "ui/section_title.h"
#include "xt/instrument.h"
#include "util/cgprint.h"
#include "palette.h"
#include "core/macro.h"
#include <string.h>

static const int16_t kbase_x = XT_UI_INSTRUMENT_LIST_X;
static const int16_t kbase_y = XT_UI_INSTRUMENT_LIST_Y;
static const int16_t kbase_w = XT_UI_INSTRUMENT_LIST_W;
static const int16_t kbase_h = XT_UI_AREA_H - kbase_y - 8;  // Space for the channel indicators
static const uint16_t klist_size = (kbase_h - XT_UI_TITLEBAR_CONTENT_OFFS_Y) / XT_UI_ROW_SPACING;

void xt_instrument_list_renderer_init(XtInstrumentListRenderer *a)
{
	memset(a, 0, sizeof(*a));
	a->instrument_id = -1;
	a->full_repaint = true;
}

static void draw_list(XtInstrumentListRenderer *a, const XtTrack *t)
{
	cgbox(XT_UI_PLANE, XT_PAL_BACK,
	      kbase_x, kbase_y + XT_UI_TITLEBAR_CONTENT_OFFS_Y,
	      kbase_w, kbase_h - XT_UI_TITLEBAR_CONTENT_OFFS_Y);
	// TODO: OPM / PCM / etc icon
	const int16_t ideal_first_id = (a->instrument_id - klist_size/2);
	const int16_t first_id = (ideal_first_id < 0) ? 0 : ideal_first_id;
	int16_t draw_y = kbase_y + XT_UI_TITLEBAR_CONTENT_OFFS_Y;

	for (uint16_t i = 0; i < klist_size; i++)
	{
		const uint16_t id = i + first_id;
		if (id >= t->num_instruments) break;
		const XtInstrument *ins = &t->instruments[id];
		const uint8_t pal = (id == a->instrument_id) ? XT_PAL_MAIN : XT_PAL_ACCENT2;
		cgprint_hex2(XT_UI_PLANE, pal,
		             kbase_x + XT_UI_INSTRUMENT_LIST_NUM_OFFS_X, draw_y, id);
		// TODO: Icon?
		cgprint(XT_UI_PLANE, pal, ins->name,
		        kbase_x + XT_UI_INSTRUMENT_LIST_NAME_OFFS_X, draw_y);
		draw_y += XT_UI_ROW_SPACING;
	}
}

// Draw the instrument table as-needed based on track data and the provided
// navigation position.
void xt_instrument_list_renderer_tick(XtInstrumentListRenderer *a,
                                      const XtTrack *t, int16_t instrument_id)
{
	if (instrument_id != a->instrument_id)
	{
		a->data_repaint = true;
		a->instrument_id = instrument_id;
	}

	if (a->full_repaint)
	{
		ui_section_title_draw("Instruments", kbase_x, kbase_y, kbase_w, kbase_h);

		// As part of a full repaint, the instrument data will be redrawn as well.
		a->full_repaint = false;
		a->data_repaint = true;
	}

	if (a->data_repaint && a->instrument_id >= 0)
	{
		draw_list(a, t);
		a->data_repaint = false;
	}
}

// Mark all frames and the border as needing a redraw.
void xt_instrument_list_renderer_request_redraw(XtInstrumentListRenderer *a,
                                                bool content_only)
{
	a->data_repaint = true;
	if (!content_only) a->full_repaint = true;
}
