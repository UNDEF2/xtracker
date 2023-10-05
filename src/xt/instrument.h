#ifndef _XT_INSTRUMENT_H
#define _XT_INSTRUMENT_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "xbase/opm.h"

#include "xt/types.h"

#include "xt/instrument_opm.h"
#include "xt/instrument_msm6258.h"
#include "xt/instrument_opn_fm.h"

typedef enum XtInstrumentType
{
	XT_INSTRUMENT_TYPE_OPM,
	XT_INSTRUMENT_TYPE_MSM6258,
	XT_INSTRUMENT_TYPE_OPN_FM,
} XtInstrumentType;

// Representation of an XtInstrument in memory.
typedef struct XtInstrument
{
	XtInstrumentType type;
	union
	{
		XtInstrumentOpm opm;
		XtInstrumentMSM6258 adpcm;
		XtInstrumentOpnFm opn_fm;
	};
	char name[16];
} XtInstrument;

#endif  // _XT_INSTRUMENT_H
