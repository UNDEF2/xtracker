#include "edit/instrument_list_editor.h"
#include "ui/fnlabels.h"
#include <string.h>

void xt_instrument_list_editor_init(XtInstrumentListEditor *a,
                                    XtInstrumentListRenderer *r)
{
	memset(a, 0, sizeof(*a));
	a->r = r;
}

static inline int16_t get_last_instrument_range_sub(const XtTrack *t)
{
	int16_t last = t->num_instruments;
	if (last > XT_INSTRUMENT_COUNT) last = XT_INSTRUMENT_COUNT;
	return last - 1;
}

static void init_instrument_opm(XtInstrument *ins)
{
	memset(ins, 0, sizeof(*ins));
	snprintf(ins->name, sizeof(ins->name), "New (OPM)");
	ins->opm.tl [0] = 127;
	ins->opm.tl [2] = 127;

	ins->opm.ar [1] = 31; // AKA A
	ins->opm.d1r[1] = 10; // AKA D
	ins->opm.d1l[1] = 2;  // AKA S
	ins->opm.d2r[1] = 4;  // AKA D2
	ins->opm.rr [1] = 15; // AKA R
	ins->opm.tl [1] = 127;
	ins->opm.mul[1] = 1;  // AKA ML

	ins->opm.ar [3] = 31; // AKA A
	ins->opm.d1r[3] = 5;  // AKA D
	ins->opm.d1l[3] = 0;  // AKA S
	ins->opm.d2r[3] = 2;  // AKA D2
	ins->opm.rr [3] = 15;  // AKA R
	ins->opm.tl [3] = 1;
	ins->opm.mul[3] = 1;  // AKA ML
}

// Moves all instruments below `from_id` upwards by one.
static void pull_instruments_up(XtTrack *t, int16_t from_id)
{
	XtInstrument *instruments = t->instruments;
	if (t->num_instruments <= 1) return;
	
	for (int16_t i = from_id; i < get_last_instrument_range_sub(t); i++)
	{
		instruments[i] = instruments[i + 1];
	}
}
// Moves all instruments below `from_id` downwards by one.
static void push_instruments_down(XtTrack *t, int16_t from_id)
{
	XtInstrument *instruments = t->instruments;
	if (t->num_instruments <= 1) return;

	for (int16_t i = get_last_instrument_range_sub(t) - 1; i >= from_id; i--)
	{
		instruments[i + 1] = instruments[i];
	}
}


void xt_instrument_list_editor_on_key(XtInstrumentListEditor *a,
                                      int16_t *instrument_id, XtTrack *t,
                                      XBKeyEvent key_event)
{
	if (key_event.modifiers & XB_KEY_MOD_KEY_UP) return;
	switch (key_event.name)
	{
		case XB_KEY_DOWN:
			*instrument_id = *instrument_id + 1;
			if (*instrument_id >= t->num_instruments)
			{
				*instrument_id = t->num_instruments - 1;
			}
			break;

		case XB_KEY_UP:
			if (*instrument_id > 0) *instrument_id = *instrument_id - 1;
			break;

		case XB_KEY_V:  // Paste
			if (!(key_event.modifiers & XB_KEY_MOD_CTRL)) break;
			if (!a->copied_instrument_valid) break;
			__attribute__((fallthrough));
		case XB_KEY_N:  // New instrument
		case XB_KEY_F1:
			// TODO: Open prompt for which instrument type.
			if (t->num_instruments >= XT_INSTRUMENT_COUNT - 1) break;
			t->num_instruments++;
			push_instruments_down(t, *instrument_id);
			// What is done to the instrument slot differs based on the paste vs new path.
			if (key_event.name != XB_KEY_V) init_instrument_opm(&t->instruments[*instrument_id]);
			else t->instruments[*instrument_id] = a->copied_instrument;
			break;

		case XB_KEY_C:  // Copy
		case XB_KEY_X:  // Cut
			if (!(key_event.modifiers & XB_KEY_MOD_CTRL)) break;
			a->copied_instrument = t->instruments[*instrument_id];
			a->copied_instrument_valid = true;
			if (key_event.name != XB_KEY_X) break;  // Continue to deletion for the cut case
			__attribute__((fallthrough));
		case XB_KEY_DEL:  // Delete instrument
		case XB_KEY_D:
		case XB_KEY_F2:
			if (t->num_instruments == 1)
			{
				*instrument_id = 0;
				init_instrument_opm(&t->instruments[*instrument_id]);
				break;
			}
			if (t->num_instruments <= 0) break;
			pull_instruments_up(t, *instrument_id);
			t->num_instruments--;
			if (t->num_instruments > 0 && *instrument_id >= t->num_instruments)
			{
				*instrument_id = t->num_instruments - 1;
			}
			break;

		default:
			return;
	}

	xt_instrument_list_renderer_request_redraw(a->r, /*content_only=*/true);
}

void xt_instrument_list_editor_set_fnlabels(void)
{
	ui_fnlabel_set(0, "New");
	ui_fnlabel_set(1, "Delete");
	ui_fnlabel_set(2, "");
	ui_fnlabel_set(3, "");
	ui_fnlabel_set(4, "");
}
