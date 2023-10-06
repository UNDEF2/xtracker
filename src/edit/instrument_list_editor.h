#ifndef EDIT_INSTRUMENT_LIST_EDITOR_H
#define EDIT_INSTRUMENT_LIST_EDITOR_H

#include <stdint.h>
#include "xbase/keys.h"
#include "xt/track.h"
#include "xt/instrument.h"
#include "ui/instrument_list_render.h"

typedef struct XtInstrumentListEditor
{
	bool copied_instrument_valid;
	XtInstrument copied_instrument;
	XtInstrumentListRenderer *r;
} XtInstrumentListEditor;

void xt_instrument_list_editor_init(XtInstrumentListEditor *a,
                                    XtInstrumentListRenderer *r);

void xt_instrument_list_editor_on_key(XtInstrumentListEditor *a,
                                      int16_t *instrument_id,
                                      XtTrack *t, XBKeyEvent key_event);

void xt_instrument_list_editor_set_fnlabels(void);

#endif  // EDIT_INSTRUMENT_LIST_EDITOR_H
