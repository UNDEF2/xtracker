#include "ui/regdata_render.h"
#include "ui/metrics.h"
#include "ui/section_title.h"
#include "xt/instrument.h"
#include "util/cgprint.h"
#include "palette.h"
#include "core/macro.h"
#include <string.h>

static const int16_t kbase_x = XT_UI_REGDATA_X;
static const int16_t kbase_y = XT_UI_REGDATA_Y;
static const int16_t kbase_w = XT_UI_AREA_W - kbase_x - XT_UI_MARGIN_SIDE;
static const int16_t kbase_h = XT_UI_AREA_H - kbase_y - 8;  // Space for the channel indicators

void xt_regdata_renderer_init(XtRegdataRenderer *a)
{
	memset(a, 0, sizeof(*a));
	a->full_repaint = true;
}

// OPM column redraw sub (two digit)
//      a: XtRegdataRenderer pointer
//    cur: current instrument column uint32_t union
//    new: new instrument     "
//      x: pixel position
//  force: force full redraw regardless of instrument data match
static inline void opm_update_col_sub(XtRegdataRenderer *a,
                                      uint32_t *cur, const uint32_t *new,
                                      uint16_t x, bool force,
                                      bool col_active)
{
	if (col_active) force = true;
	if (!force && *cur == *new) return;
	const int16_t hl_row = (a->edit.active && col_active) ? a->edit.row : -1;
	*cur = *new;
	uint16_t y = kbase_y + XT_UI_REGDATA_TABLE_OFFS_Y + XT_UI_ROW_SPACING;
	const uint8_t *data_u8 = (const uint8_t *)new;
	for (uint16_t i = 0; i < XB_OPM_OP_COUNT; i++)
	{
		const uint8_t pal = (!a->edit.active || i == hl_row)
		                    ? XT_PAL_MAIN : XT_PAL_ACCENT2;
		cgprint_hex2(XT_UI_PLANE, pal, x, y, data_u8[i]);
		y += XT_UI_ROW_SPACING;
	}
	// muck up the cached data to force a redraw
	if (col_active) *cur = ~*cur;
}

static void opm_reg_paint(XtRegdataRenderer *a, const XtInstrumentOpm *new, bool force)
{
	XtInstrumentOpm *cur = &a->data.opm;
	// Left column
	if (a->edit.active || force || cur->con != new->con)
	{
		const uint8_t pal = (!a->edit.active || (a->edit.row == 0 && a->edit.column == 0))
		                    ? XT_PAL_MAIN : XT_PAL_ACCENT2;
		cur->con = new->con;
		cgprint_hex1(XT_UI_PLANE, pal, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_CON_OFFS_Y,
		             new->con);

		static const uint16_t con_tileidx[] =
		{
			0x100, 0x110, 0x120, 0x130, 0x140, 0x150, 0x160, 0x170
		};
		cgtile(XT_UI_PLANE,
		       kbase_x + XT_UI_REGDATA_CONCHART_OFFS_X,
		       kbase_y + XT_UI_REGDATA_CONCHART_OFFS_Y,
		       con_tileidx[new->con], 4, 4);
	}
	if (a->edit.active || force || cur->fl != new->fl)
	{
		const uint8_t pal = (!a->edit.active || (a->edit.row == 1 && a->edit.column == 0))
		                    ? XT_PAL_MAIN : XT_PAL_ACCENT2;
		cur->fl = new->fl;
		cgprint_hex1(XT_UI_PLANE, pal, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_FL_OFFS_Y,
		             new->fl);
	}
	if (a->edit.active || force || cur->pms != new->pms)
	{
		const uint8_t pal = (!a->edit.active || (a->edit.row == 2 && a->edit.column == 0))
		                    ? XT_PAL_MAIN : XT_PAL_ACCENT2;
		cur->pms = new->pms;
		cgprint_hex1(XT_UI_PLANE, pal, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_PMS_OFFS_Y,
		             new->pms);
	}
	if (a->edit.active || force || cur->ams != new->ams)
	{
		const uint8_t pal = (!a->edit.active || (a->edit.row == 3 && a->edit.column == 0))
		                    ? XT_PAL_MAIN : XT_PAL_ACCENT2;
		cur->ams = new->ams;
		cgprint_hex1(XT_UI_PLANE, pal, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_AMS_OFFS_Y,
		             new->ams);
	}

	// The table, updated one column at a time to speed up comparisons
	_Static_assert(sizeof(uint32_t) == XB_OPM_OP_COUNT);
	uint16_t left_x = kbase_x + XT_UI_REGDATA_TABLE_OFFS_X + 11;

	opm_update_col_sub(a, &cur->ar_u32,  &new->ar_u32,  left_x, force, a->edit.column == 1);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->d1r_u32, &new->d1r_u32, left_x, force, a->edit.column == 2);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->d2r_u32, &new->d2r_u32, left_x, force, a->edit.column == 3);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->rr_u32,  &new->rr_u32,  left_x, force, a->edit.column == 4);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->d1l_u32, &new->d1l_u32, left_x, force, a->edit.column == 5);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->tl_u32,  &new->tl_u32,  left_x, force, a->edit.column == 6);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->ks_u32,  &new->ks_u32,  left_x, force, a->edit.column == 7);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->mul_u32, &new->mul_u32, left_x, force, a->edit.column == 8);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->dt1_u32, &new->dt1_u32, left_x, force, a->edit.column == 9);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->dt2_u32, &new->dt2_u32, left_x, force, a->edit.column == 10);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(a, &cur->ame_u32, &new->ame_u32, left_x, force, a->edit.column == 11);
	left_x += XT_UI_COL_SPACING;
}

static void opm_backing_paint(void)
{
	// Left data (non-op-specific)
	const uint32_t colors = (XT_PAL_BACK << 16) | XT_PAL_ACCENT2;
	cgprint_noalpha(XT_UI_PLANE, colors, "YM2151",
	                kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_TYPE_OFFS_Y);
	cgprint_noalpha(XT_UI_PLANE, colors, "CON",
	                kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_CON_OFFS_Y);
	cgprint_noalpha(XT_UI_PLANE, colors, "FL",
	                kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_FL_OFFS_Y);
	cgprint_noalpha(XT_UI_PLANE, colors, "PMS",
	                kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_PMS_OFFS_Y);
	cgprint_noalpha(XT_UI_PLANE, colors, "AMS",
	                kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_AMS_OFFS_Y);

	// Draw the register labels as direct graphics
	cgtile(XT_UI_PLANE,
	       kbase_x + XT_UI_REGDATA_TABLE_OFFS_X, kbase_y + XT_UI_REGDATA_TABLE_OFFS_Y,
	       0xE0, 23, 1);
	
	// OPerator numbers for the table (in funky order)
	static const uint8_t kop_lut[4] = {1, 3, 2, 4};
	for (uint16_t i = 0; i < ARRAYSIZE(kop_lut); i++)
	{
		cgprint_hex1(XT_UI_PLANE, XT_PAL_ACCENT1,
		             kbase_x + XT_UI_REGDATA_TABLE_OFFS_X,
		             kbase_y + (XT_UI_ROW_SPACING * (1 + i)) + XT_UI_REGDATA_TABLE_OFFS_Y,
		             kop_lut[i]);
		
	}
}

// Draw the instrument table as-needed based on track data and the provided
// navigation position.
void xt_regdata_renderer_tick(XtRegdataRenderer *a, const XtInstrument *ins)
{
	// If the current instrument type differs from the last, do a full redraw.
	if (ins->type != a->data.type)
	{
		a->full_repaint = true;
		a->data.type = ins->type;
	}

	if (a->full_repaint)
	{
		switch (ins->type)
		{
			case XT_INSTRUMENT_TYPE_OPM:
				ui_section_title_draw("Instrument Data (OPM)", kbase_x, kbase_y, kbase_w, kbase_h);
				memset(&a->data.opm, 0xFF, sizeof(a->data.opm));
				opm_backing_paint();
				opm_reg_paint(a, &ins->opm, true);
				break;
			case XT_INSTRUMENT_TYPE_MSM6258:
				ui_section_title_draw("Instrument Data (ADPCM)", kbase_x, kbase_y, kbase_w, kbase_h);
				break;
			case XT_INSTRUMENT_TYPE_OPN_FM:
				ui_section_title_draw("Instrument Data (OPN-FM)", kbase_x, kbase_y, kbase_w, kbase_h);
				break;
		}

		// As part of a full repaint, the instrument data will be redrawn as well.
		a->full_repaint = false;
		a->data_repaint = true;
	}

	// Repaint any data that does not match what is current.
	switch (ins->type)
	{
		case XT_INSTRUMENT_TYPE_OPM:
			opm_reg_paint(a, &ins->opm, a->data_repaint);
			break;
		default:
			break;
	}
	a->data_repaint = false;
}

// Mark all frames and the border as needing a redraw.
void xt_regdata_renderer_request_redraw(XtRegdataRenderer *a, bool content_only)
{
	a->data_repaint = true;
	if (!content_only) a->full_repaint = true;
}

void xt_regdata_renderer_enable_edit_cursor(XtRegdataRenderer *s, bool active)
{
	s->edit.active = active;
}

void xt_regdata_renderer_set_edit_cursor(XtRegdataRenderer *s, int16_t row, int16_t column)
{
	s->edit.row = row;
	s->edit.column = column;
}
