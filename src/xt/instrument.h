#ifndef _XT_INSTRUMENT_H
#define _XT_INSTRUMENT_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "xbase/opm.h"

#include "xt/types.h"

typedef enum XtChannelType
{
	XT_CHANNEL_OPM,
	XT_CHANNEL_ADPCM
} XtChannelType;

typedef struct XtOpmPatch
{
	uint8_t con;
	uint8_t fl;
	uint8_t pms;
	uint8_t ams;
	union
	{
		uint8_t ar[XB_OPM_OP_COUNT];
		uint32_t ar_u32;
	};
	union
	{
		uint8_t d1r[XB_OPM_OP_COUNT];
		uint32_t d1r_u32;
	};
	union
	{
		uint8_t d2r[XB_OPM_OP_COUNT];
		uint32_t d2r_u32;
	};
	union
	{
		uint8_t rr[XB_OPM_OP_COUNT];
		uint32_t rr_u32;
	};
	union
	{
		uint8_t d1l[XB_OPM_OP_COUNT];
		uint32_t d1l_u32;
	};
	union
	{
		uint8_t tl[XB_OPM_OP_COUNT];
		uint32_t tl_u32;
	};
	union
	{
		uint8_t ks[XB_OPM_OP_COUNT];
		uint32_t ks_u32;
	};
	union
	{
		uint8_t mul[XB_OPM_OP_COUNT];
		uint32_t mul_u32;
	};
	union
	{
		uint8_t dt1[XB_OPM_OP_COUNT];
		uint32_t dt1_u32;
	};
	union
	{
		uint8_t dt2[XB_OPM_OP_COUNT];
		uint32_t dt2_u32;
	};
	union
	{
		uint8_t ame[XB_OPM_OP_COUNT];
		uint32_t ame_u32;
	};
} XtOpmPatch;

typedef struct XtAdpcmPatch
{
	// TODO: all of this
} XtAdpcmPatch;

// Representation of an XtInstrument in memory.
struct XtInstrument
{
	XtChannelType type;
	union
	{
		XtOpmPatch opm;
		// TODO: OPN, OPL support? Depends on platforms and/or exp hardware.
		XtAdpcmPatch adpcm;
	};
	char name[16];
	// TODO: Vibrato / pitch mod params, etc.
};
typedef struct XtInstrument XtInstrument;

struct XtInstrumentFileRecord
{
	uint8_t header_magic[8];
	XtInstrument instrument_data;

	// Array of indeterminate length containing ADPCM sample data.
	int16_t adpcm_data_embedded;
	uint32_t adpcm_data_length;
	uint8_t adpcm_data[0];
} __attribute__((packed)) __attribute__((aligned(2)));
typedef struct XtInstrumentRecord XtInstrumentRecord;

XtResult xt_instrument_load_from_file(FILE *f, XtInstrument *out);
XtResult xt_instrument_write_to_file(const XtInstrument *in, FILE *f);

#endif  // _XT_INSTRUMENT_H
