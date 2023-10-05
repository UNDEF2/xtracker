#ifndef EDIT_INSTRUMENT_EDITOR_H
#define EDIT_INSTRUMENT_EDITOR_H

#include <stdint.h>
#include "xbase/keys.h"
#include "xt/track.h"
#include "ui/regdata_render.h"
#include "xt/instrument.h"

// Enum that maps to the register grid for the OPM instrument register view.
typedef enum XtInstrumentEditorOpmPos
{
	XT_IEDP_CON, XT_IEDP_AR1, XT_IEDP_DR1, XT_IEDP_SR1, XT_IEDP_RR1, XT_IEDP_SL1, XT_IEDP_TL1, XT_IEDP_KS1, XT_IEDP_ML1, XT_IEDP_DT11, XT_IEDP_DT21, XT_IEDP_AME1,
	XT_IEDP_FL,  XT_IEDP_AR3, XT_IEDP_DR3, XT_IEDP_SR3, XT_IEDP_RR3, XT_IEDP_SL3, XT_IEDP_TL3, XT_IEDP_KS3, XT_IEDP_ML3, XT_IEDP_DT13, XT_IEDP_DT23, XT_IEDP_AME3,
	XT_IEDP_PMS, XT_IEDP_AR2, XT_IEDP_DR2, XT_IEDP_SR2, XT_IEDP_RR2, XT_IEDP_SL2, XT_IEDP_TL2, XT_IEDP_KS2, XT_IEDP_ML2, XT_IEDP_DT12, XT_IEDP_DT22, XT_IEDP_AME2,
	XT_IEDP_AMS, XT_IEDP_AR4, XT_IEDP_DR4, XT_IEDP_SR4, XT_IEDP_RR4, XT_IEDP_SL4, XT_IEDP_TL4, XT_IEDP_KS4, XT_IEDP_ML4, XT_IEDP_DT14, XT_IEDP_DT24, XT_IEDP_AME4,
	XT_IEDP_COUNT
} XtInstrumentEditorOpmPos;

typedef struct XtInstrumentEditor
{
	uint8_t num;
	uint8_t entry_pos;

	struct
	{
		uint8_t ar;
		uint8_t d1r;
		uint8_t d2r;
		uint8_t rr;
		uint8_t d1l;
		uint8_t tl;
		uint8_t ks;
		uint8_t mul;
		uint8_t dt1;
		uint8_t dt2;
		uint8_t ame;
	} opm_op_buffer;
	XtInstrumentEditorOpmPos pos;
	XtRegdataRenderer *r;
} XtInstrumentEditor;

void xt_instrument_editor_init(XtInstrumentEditor *a, XtRegdataRenderer *r);

void xt_instrument_editor_on_key(XtInstrumentEditor *a, int16_t instrument_id, XtTrack *t, XBKeyEvent e);

void xt_instrument_editor_set_fnlabels(void);


#endif  // EDIT_INSTRUMENT_EDITOR_H
