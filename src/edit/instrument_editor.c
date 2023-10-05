#include "edit/instrument_editor.h"
#include "core/macro.h"
#include "ui/fnlabels.h"

#include <string.h>

//
// Grid position helpers
//

static const int16_t kcol_tbl[] =
{
	[XT_IEDP_CON] = 0,
	[XT_IEDP_FL] = 0,
	[XT_IEDP_PMS] = 0,
	[XT_IEDP_AMS] = 0,

	[XT_IEDP_AR1] = 1,
	[XT_IEDP_AR3] = 1,
	[XT_IEDP_AR2] = 1,
	[XT_IEDP_AR4] = 1,

	[XT_IEDP_DR1] = 2,
	[XT_IEDP_DR3] = 2,
	[XT_IEDP_DR2] = 2,
	[XT_IEDP_DR4] = 2,

	[XT_IEDP_SR1] = 3,
	[XT_IEDP_SR3] = 3,
	[XT_IEDP_SR2] = 3,
	[XT_IEDP_SR4] = 3,

	[XT_IEDP_RR1] = 4,
	[XT_IEDP_RR3] = 4,
	[XT_IEDP_RR2] = 4,
	[XT_IEDP_RR4] = 4,

	[XT_IEDP_SL1] = 5,
	[XT_IEDP_SL3] = 5,
	[XT_IEDP_SL2] = 5,
	[XT_IEDP_SL4] = 5,

	[XT_IEDP_TL1] = 6,
	[XT_IEDP_TL3] = 6,
	[XT_IEDP_TL2] = 6,
	[XT_IEDP_TL4] = 6,

	[XT_IEDP_KS1] = 7,
	[XT_IEDP_KS3] = 7,
	[XT_IEDP_KS2] = 7,
	[XT_IEDP_KS4] = 7,

	[XT_IEDP_ML1] = 8,
	[XT_IEDP_ML3] = 8,
	[XT_IEDP_ML2] = 8,
	[XT_IEDP_ML4] = 8,

	[XT_IEDP_DT11] = 9,
	[XT_IEDP_DT13] = 9,
	[XT_IEDP_DT12] = 9,
	[XT_IEDP_DT14] = 9,

	[XT_IEDP_DT21] = 10,
	[XT_IEDP_DT23] = 10,
	[XT_IEDP_DT22] = 10,
	[XT_IEDP_DT24] = 10,

	[XT_IEDP_AME1] = 11,
	[XT_IEDP_AME3] = 11,
	[XT_IEDP_AME2] = 11,
	[XT_IEDP_AME4] = 11
};

static inline int16_t row_from_pos(XtInstrumentEditorOpmPos pos)
{
	if (pos <= XT_IEDP_AME1) return 0;
	if (pos <= XT_IEDP_AME3) return 1;
	if (pos <= XT_IEDP_AME2) return 2;
	if (pos <= XT_IEDP_AME4) return 3;
	return 0;
}

static inline int16_t col_from_pos(XtInstrumentEditorOpmPos pos)
{
	if (pos < 0 || pos >= XT_IEDP_COUNT) return 0;
	return kcol_tbl[pos];
}

//
// Navigation
//

static inline void cursor_move_next(XtInstrumentEditor *a)
{
	a->pos++;
	if (a->pos >= XT_IEDP_COUNT) a->pos = XT_IEDP_COUNT - 1;
}

static inline void cursor_move_prev(XtInstrumentEditor *a)
{
	if (a->pos > XT_IEDP_CON) a->pos--;
}

//
// Number entry
//

typedef struct XtKeyNumberPairing
{
	XBKey key;
	int8_t value;
} XtKeyNumberPairing;

static XtKeyNumberPairing knumber_key_lookup[] =
{
	{XB_KEY_0, 0},
	{XB_KEY_1, 1},
	{XB_KEY_2, 2},
	{XB_KEY_3, 3},
	{XB_KEY_4, 4},
	{XB_KEY_5, 5},
	{XB_KEY_6, 6},
	{XB_KEY_7, 7},
	{XB_KEY_8, 8},
	{XB_KEY_9, 9},
	{XB_KEY_A, 0xA},
	{XB_KEY_B, 0xB},
	{XB_KEY_C, 0xC},
	{XB_KEY_D, 0xD},
	{XB_KEY_E, 0xE},
	{XB_KEY_F, 0xF},
	{XB_KEY_NUMPAD_0, 0},
	{XB_KEY_NUMPAD_1, 1},
	{XB_KEY_NUMPAD_2, 2},
	{XB_KEY_NUMPAD_3, 3},
	{XB_KEY_NUMPAD_4, 4},
	{XB_KEY_NUMPAD_5, 5},
	{XB_KEY_NUMPAD_6, 6},
	{XB_KEY_NUMPAD_7, 7},
	{XB_KEY_NUMPAD_8, 8},
	{XB_KEY_NUMPAD_9, 9},
	// TODO
//	{XB_KEY_MINUS, -1},
//	{XB_KEY_NUMPAD_MINUS, -1},
};

// Returns true if a key entry was accepted.
static inline bool number_entry_opm(XtInstrumentEditor *a, XtOpmPatch *opm, XBKey key)
{
	typedef struct Spec
	{
		int8_t min, max;
		uint8_t *data;
		uint8_t digits;
		bool is_signed;
	} Spec;

	Spec spec;

	switch (a->pos)
	{
		default:
			break;
		case XT_IEDP_CON:  spec = (Spec){0, 7,  &opm->con , 1, false}; break;
		case XT_IEDP_AR1:  spec = (Spec){0, 31, &opm->ar[0], 2, false}; break;
		case XT_IEDP_DR1:  spec = (Spec){0, 31, &opm->d1r[0], 2, false}; break;
		case XT_IEDP_SR1:  spec = (Spec){0, 31, &opm->d2r[0], 2, false}; break;
		case XT_IEDP_RR1:  spec = (Spec){0, 15, &opm->rr[0], 1, false}; break;
		case XT_IEDP_SL1:  spec = (Spec){0, 15, &opm->d1l[0], 1, false}; break;
		case XT_IEDP_TL1:  spec = (Spec){0, 127,&opm->tl[0], 2, false}; break;
		case XT_IEDP_KS1:  spec = (Spec){0, 3,  &opm->ks[0], 1, false}; break;
		case XT_IEDP_ML1:  spec = (Spec){0, 15, &opm->mul[0], 1, false}; break;
		case XT_IEDP_DT11: spec = (Spec){-3, 4, &opm->dt1[0],1, true}; break;
		case XT_IEDP_DT21: spec = (Spec){0, 3,  &opm->dt2[0],1, false}; break;
		case XT_IEDP_AME1: spec = (Spec){0, 1,  &opm->ame[0],1, false}; break;

		case XT_IEDP_FL:   spec = (Spec){0, 7,  &opm->fl   , 1, false}; break;
		case XT_IEDP_AR3:  spec = (Spec){0, 31, &opm->ar[1], 2, false}; break;
		case XT_IEDP_DR3:  spec = (Spec){0, 31, &opm->d1r[1], 2, false}; break;
		case XT_IEDP_SR3:  spec = (Spec){0, 31, &opm->d2r[1], 2, false}; break;
		case XT_IEDP_RR3:  spec = (Spec){0, 15, &opm->rr[1], 1, false}; break;
		case XT_IEDP_SL3:  spec = (Spec){0, 15, &opm->d1l[1], 1, false}; break;
		case XT_IEDP_TL3:  spec = (Spec){0, 127,&opm->tl[1], 2, false}; break;
		case XT_IEDP_KS3:  spec = (Spec){0, 3,  &opm->ks[1], 1, false}; break;
		case XT_IEDP_ML3:  spec = (Spec){0, 15, &opm->mul[1], 1, false}; break;
		case XT_IEDP_DT13: spec = (Spec){-3, 4, &opm->dt1[1],1, true}; break;
		case XT_IEDP_DT23: spec = (Spec){0, 3,  &opm->dt2[1],1, false}; break;
		case XT_IEDP_AME3: spec = (Spec){0, 1,  &opm->ame[1],1, false}; break;

		case XT_IEDP_PMS:  spec = (Spec){0, 7,  &opm->pms  , 1, false}; break;
		case XT_IEDP_AR2:  spec = (Spec){0, 31, &opm->ar[2], 2, false}; break;
		case XT_IEDP_DR2:  spec = (Spec){0, 31, &opm->d1r[2], 2, false}; break;
		case XT_IEDP_SR2:  spec = (Spec){0, 31, &opm->d2r[2], 2, false}; break;
		case XT_IEDP_RR2:  spec = (Spec){0, 15, &opm->rr[2], 1, false}; break;
		case XT_IEDP_SL2:  spec = (Spec){0, 15, &opm->d1l[2], 1, false}; break;
		case XT_IEDP_TL2:  spec = (Spec){0, 127,&opm->tl[2], 2, false}; break;
		case XT_IEDP_KS2:  spec = (Spec){0, 3,  &opm->ks[2], 1, false}; break;
		case XT_IEDP_ML2:  spec = (Spec){0, 15, &opm->mul[2], 1, false}; break;
		case XT_IEDP_DT12: spec = (Spec){-3, 4, &opm->dt1[2],1, true}; break;
		case XT_IEDP_DT22: spec = (Spec){0, 3,  &opm->dt2[2],1, false}; break;
		case XT_IEDP_AME2: spec = (Spec){0, 1,  &opm->ame[2],1, false}; break;

		case XT_IEDP_AMS:  spec = (Spec){0, 3,  &opm->ams  , 1, false}; break;
		case XT_IEDP_AR4:  spec = (Spec){0, 31, &opm->ar[3], 2, false}; break;
		case XT_IEDP_DR4:  spec = (Spec){0, 31, &opm->d1r[3], 2, false}; break;
		case XT_IEDP_SR4:  spec = (Spec){0, 31, &opm->d2r[3], 2, false}; break;
		case XT_IEDP_RR4:  spec = (Spec){0, 15, &opm->rr[3], 1, false}; break;
		case XT_IEDP_SL4:  spec = (Spec){0, 15, &opm->d1l[3], 1, false}; break;
		case XT_IEDP_TL4:  spec = (Spec){0, 127,&opm->tl[3], 2, false}; break;
		case XT_IEDP_KS4:  spec = (Spec){0, 3,  &opm->ks[3], 1, false}; break;
		case XT_IEDP_ML4:  spec = (Spec){0, 15, &opm->mul[3], 1, false}; break;
		case XT_IEDP_DT14: spec = (Spec){-3, 4, &opm->dt1[3],1, true}; break;
		case XT_IEDP_DT24: spec = (Spec){0, 3,  &opm->dt2[3],1, false}; break;
		case XT_IEDP_AME4: spec = (Spec){0, 1,  &opm->ame[3],1, false}; break;
	}

	// Non-numerical entries
	switch (key)
	{
		default:
			break;
		case XB_KEY_NUMPAD_PLUS:
		case XB_KEY_OPEN_BRACKET:
			*spec.data = *spec.data + 1;
			goto modified;
		case XB_KEY_NUMPAD_MINUS:
		case XB_KEY_CLOSED_BRACKET:
			if (*spec.data > 0) *spec.data = *spec.data - 1;
			goto modified;
		case XB_KEY_F1:
			*spec.data = spec.min;
			goto modified;
		case XB_KEY_F2:
			*spec.data = spec.max;
			goto modified;
	}


	for (uint16_t i = 0; i < ARRAYSIZE(knumber_key_lookup); i++)
	{
		if (key != knumber_key_lookup[i].key) continue;
		const int8_t entry_val = knumber_key_lookup[i].value;
		if (a->entry_pos == 1 || spec.digits == 1)
		{
			*spec.data &= 0xF0;
			*spec.data |= entry_val & 0x0F;
			a->entry_pos = 0;
			cursor_move_next(a);
			goto modified;
		}
		else if (a->entry_pos == 0)
		{
			*spec.data &= 0x0F;
			*spec.data |= (entry_val << 4) & 0xF0;
			a->entry_pos++;
			goto modified;
		}
	}
	
	return false;

modified:
	if (*spec.data > spec.max) *spec.data = spec.max;
	else if (*spec.data < spec.min) *spec.data = spec.min;
	return true;
}

static void copy_opm(XtInstrumentEditor *a, XtOpmPatch *opm, int16_t row)
{
	if (row < 0 || row > ARRAYSIZE(opm->ar)) return;
	if (col_from_pos(a->pos) == 0) return;
	a->opm_op_buffer.ar = opm->ar[row];
	a->opm_op_buffer.d1r = opm->d1r[row];
	a->opm_op_buffer.d2r = opm->d2r[row];
	a->opm_op_buffer.rr = opm->rr[row];
	a->opm_op_buffer.d1l = opm->d1l[row];
	a->opm_op_buffer.tl = opm->tl[row];
	a->opm_op_buffer.ks = opm->ks[row];
	a->opm_op_buffer.mul = opm->mul[row];
	a->opm_op_buffer.dt1 = opm->dt1[row];
	a->opm_op_buffer.dt2 = opm->dt2[row];
	a->opm_op_buffer.ame = opm->ame[row];
}

static void paste_opm(XtInstrumentEditor *a, XtOpmPatch *opm, int16_t row)
{
	if (row < 0 || row > ARRAYSIZE(opm->ar)) return;
	if (col_from_pos(a->pos) == 0) return;
	opm->ar[row] = a->opm_op_buffer.ar;
	opm->d1r[row] = a->opm_op_buffer.d1r;
	opm->d2r[row] = a->opm_op_buffer.d2r;
	opm->rr[row] = a->opm_op_buffer.rr;
	opm->d1l[row] = a->opm_op_buffer.d1l;
	opm->tl[row] = a->opm_op_buffer.tl;
	opm->ks[row] = a->opm_op_buffer.ks;
	opm->mul[row] = a->opm_op_buffer.mul;
	opm->dt1[row] = a->opm_op_buffer.dt1;
	opm->dt2[row] = a->opm_op_buffer.dt2;
	opm->ame[row] = a->opm_op_buffer.ame;
}

static void on_key_opm(XtInstrumentEditor *a, XtOpmPatch *opm, XtTrack *t, XBKeyEvent e)
{
	if (!(e.modifiers & XB_KEY_MOD_CTRL) && (number_entry_opm(a, opm, e.name))) return;

	switch (e.name)
	{
		default:
			break;
		case XB_KEY_RIGHT:
			cursor_move_next(a);
			break;
		case XB_KEY_LEFT:
			cursor_move_prev(a);
			break;
		case XB_KEY_UP:
			if (a->pos >= XT_IEDP_FL) a->pos -= XT_IEDP_FL;
			break;
		case XB_KEY_DOWN:
			if (a->pos <= XT_IEDP_AME2) a->pos += XT_IEDP_FL;
			break;
		case XB_KEY_C:
			if (!(e.modifiers & XB_KEY_MOD_CTRL)) break;
		case XB_KEY_F3:
			copy_opm(a, opm, row_from_pos(a->pos));
			break;
		case XB_KEY_V:
			if (!(e.modifiers & XB_KEY_MOD_CTRL)) break;
		case XB_KEY_F4:
			paste_opm(a, opm, row_from_pos(a->pos));
			break;
	}
}

//
// Interface
//

void xt_instrument_editor_init(XtInstrumentEditor *a, XtRegdataRenderer *r)
{
	memset(a, 0, sizeof(*a));
	a->r = r;
}

void xt_instrument_editor_on_key(XtInstrumentEditor *a, int16_t instrument_id, XtTrack *t, XBKeyEvent e)
{
	if (e.modifiers & XB_KEY_MOD_KEY_UP) return;
	const XtInstrumentEditorOpmPos pos_prev = a->pos;
	XtInstrument *ins = &t->instruments[instrument_id];

	switch (ins->type)
	{
		case XT_CHANNEL_OPM:
			on_key_opm(a, &ins->opm, t, e);
			break;
		case XT_CHANNEL_ADPCM:
			return;
	}

	if (a->pos != pos_prev)
	{
		a->entry_pos = 0;
	}

	// TOOD: row/col table for this?
	xt_regdata_renderer_set_edit_cursor(a->r, row_from_pos(a->pos), col_from_pos(a->pos));
	xt_regdata_renderer_enable_edit_cursor(a->r, true);

}

void xt_instrument_editor_set_fnlabels(void)
{
	ui_fnlabel_set(0, "Set Min");
	ui_fnlabel_set(1, "Set Max");
	ui_fnlabel_set(2, "Copy Op");
	ui_fnlabel_set(3, "Paste");
	ui_fnlabel_set(4, "");
}
