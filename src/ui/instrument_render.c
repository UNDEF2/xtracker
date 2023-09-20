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
	a->draw_x = XT_INSTRUMENT_RENDER_X;
	a->draw_y = XT_INSTRUMENT_RENDER_Y;
}

// Draw the instrumentment table as-needed based on track data and the provided
// navigation position.
void xt_instrument_renderer_tick(XtInstrumentRenderer *a, XtTrack *t, XtPhraseEditor *p, int16_t instr_num)
{
	return;
	if (!a->border_drawn)
	{
		// Legend occupies the top row of the box
		static const char kinstr_legend[] = "    AR DR SR RR SL TL KS ML DT1DT2AME";
		cgprint(0, XT_PAL_UI_HIGHLIGHT_FG, kinstr_legend, a->draw_x +
		        XT_UI_COL_SPACING, a->draw_y + XT_UI_ROW_SPACING);

		// TODO: Generalize this; this is fucking horrible copypasta from arrange
		// TODO: Customize based on instrument type
		static const char *left_mapping[] =
		{
			"\x05", "\x03", "\x03", "\x03", "\x03", "\x03", "\x07"
		};
		static const char *right_mapping[] =
		{
			"\x04", "\x03", "\x03", "\x03", "\x03", "\x03", "\x06"
		};
		int16_t y = a->draw_y;
		const int16_t left_x = a->draw_x;
		const int16_t right_x = a->draw_x + ARRAYSIZE(kinstr_legend) * XT_UI_COL_SPACING;
		y += XT_UI_ROW_SPACING;

		for (int16_t i = 0; i < ARRAYSIZE(left_mapping); i++)
		{
			cgprint(0, XT_PAL_UI_BORDER, left_mapping[i], left_x, y);
			cgprint(0, XT_PAL_UI_BORDER, right_mapping[i], right_x, y);
			y += XT_UI_ROW_SPACING;
		}

		// Operator markings for each row
		static const char *kop_legend[] =
		{
			"OP1", "OP3", "OP2", "OP4",
		};

		for (int16_t i = 0; i < XB_OPM_OP_COUNT; i++)
		{
			cgprint(0, XT_PAL_UI_HIGHLIGHT_FG, kop_legend[i], a->draw_x + XT_UI_COL_SPACING, a->draw_y + (i + 1) * XT_UI_ROW_SPACING);
			y += XT_UI_ROW_SPACING;
		}

		a->border_drawn = true;
	}

	if (a->instr_num != instr_num)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "Instrument $%02X", instr_num);

		cgprint(0, XT_PAL_UI_BORDER, buffer, a->draw_x + 6, a->draw_y);
		a->instr_num = instr_num;
	}
}

// Mark all frames and the border as needing a redraw.
void xt_instrument_renderer_redraw(XtInstrumentRenderer *a)
{
	a->instr_num = -1;
}
