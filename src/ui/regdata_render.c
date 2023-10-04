#include "ui/regdata_render.h"
#include "ui/metrics.h"
#include "ui/section_title.h"
#include "xt/instrument.h"
#include "util/cgprint.h"
#include "palette.h"
#include "common.h"
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

static inline void opm_update_col_sub(uint32_t *cur, const uint32_t *new,
                                      uint16_t x, bool force)
{
	if (!force && *cur == *new) return;
	*cur = *new;
	uint16_t y = kbase_y + XT_UI_REGDATA_TABLE_OFFS_Y + XT_UI_ROW_SPACING;
	const uint8_t *data_u8 = (const uint8_t *)new;
	for (uint16_t i = 0; i < XB_OPM_OP_COUNT; i++)
	{
		cgprint_hex2(XT_UI_PLANE, XT_PAL_MAIN, x, y, data_u8[i]);
		y += XT_UI_ROW_SPACING;
	}
}

static void opm_reg_paint(XtOpmPatch *cur, const XtOpmPatch *new, bool force)
{
	// Left column
	if (force || cur->fl != new->fl)
	{
		cur->fl = new->fl;
		cgprint_hex1(XT_UI_PLANE, XT_PAL_MAIN, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_FL_OFFS_Y,
		             new->fl);
	}
	if (force || cur->con != new->con)
	{
		cur->con = new->con;
		cgprint_hex1(XT_UI_PLANE, XT_PAL_MAIN, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
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
	if (force || cur->pms != new->pms)
	{
		cur->pms = new->pms;
		cgprint_hex1(XT_UI_PLANE, XT_PAL_MAIN, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_PMS_OFFS_Y,
		             new->pms);
	}
	if (force || cur->ams != new->ams)
	{
		cur->ams = new->ams;
		cgprint_hex1(XT_UI_PLANE, XT_PAL_MAIN, kbase_x + XT_UI_REGDATA_LEFTDAT_NUM_OFFS_X,
		             kbase_y + XT_UI_REGDATA_AMS_OFFS_Y,
		             new->ams);
	}

	// The table, updated one column at a time to speed up comparisons
	_Static_assert(sizeof(uint32_t) == XB_OPM_OP_COUNT);
	uint16_t left_x = kbase_x + XT_UI_REGDATA_TABLE_OFFS_X + 11;
	opm_update_col_sub(&cur->ar_u32, &new->ar_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->d1r_u32, &new->d1r_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->d2r_u32, &new->d2r_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->rr_u32, &new->rr_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->d1l_u32, &new->d1l_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->tl_u32, &new->tl_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->ks_u32, &new->ks_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->mul_u32, &new->mul_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->dt1_u32, &new->dt1_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->dt2_u32, &new->dt2_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
	opm_update_col_sub(&cur->ame_u32, &new->ame_u32, left_x, force);
	left_x += XT_UI_COL_SPACING;
}

static void opm_backing_paint(void)
{
	// Left data (non-op-specific)
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT2, "YM2151",
	        kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_TYPE_OFFS_Y);
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT2, "CON",
	        kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_CON_OFFS_Y);
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT2, "FL",
	        kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_FL_OFFS_Y);
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT2, "PMS",
	        kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_PMS_OFFS_Y);
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT2, "AMS",
	        kbase_x + XT_UI_REGDATA_LEFTDAT_OFFS_X, kbase_y + XT_UI_REGDATA_AMS_OFFS_Y);

	// Draw the register labels as direct graphics
	cgtile(XT_UI_PLANE,
	       kbase_x + XT_UI_REGDATA_TABLE_OFFS_X, kbase_y + XT_UI_REGDATA_TABLE_OFFS_Y,
	       0xE0, 23, 1);
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
			case XT_CHANNEL_OPM:
				ui_section_title_draw("Instrument Data (OPM)", kbase_x, kbase_y, kbase_w, kbase_h);
				memset(&a->data.opm, 0xFF, sizeof(a->data.opm));
				opm_backing_paint();
				opm_reg_paint(&a->data.opm, &ins->opm, true);
				break;
			case XT_CHANNEL_ADPCM:
				ui_section_title_draw("Instrument Data (ADPCM)", kbase_x, kbase_y, kbase_w, kbase_h);
				break;
		}

		// As part of a full repaint, the instrument data will be redrawn as well.
		a->full_repaint = false;
		a->data_repaint = true;
	}

	// Repaint any data that does not match what is current.
	switch (ins->type)
	{
		case XT_CHANNEL_OPM:
			opm_reg_paint(&a->data.opm, &ins->opm, a->data_repaint);
			break;
		case XT_CHANNEL_ADPCM:
			// TODO: ADPCM painter
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
