#ifndef _XT_INSTRUMENT_H
#define _XT_INSTRUMENT_H

#include <stdint.h>
#include <stdio.h>

#include "xt_types.h"

typedef enum XtChannelType
{
	XT_CHANNEL_OPM,
	XT_CHANNEL_ADPCM
} XtChannelType;

typedef struct XtOpmPatch
{
	uint8_t pan_fl_con;  // Rch (1), Lch(1), FL(3), CON(3)
	uint8_t pms_ams;     // null (1), PMS(3), null(2), AMS(2)
	uint8_t dt1_mul[4];  // null (1), DT1(3), MUL(4)
	uint8_t tl[4];       // null(1), TL(7)
	uint8_t ks_ar[4];    // ks(2), null(1), AR(5)
	uint8_t ame_d1r[4];  // ame(1), null(2), D1R(5)
	uint8_t dt2_d2r[4];  // dt2(2), null(1), D2R(5)
	uint8_t d1l_rr[4];   // D1L(4), RR(4)
} XtOpmPatch;

typedef struct XtAdpcmPatch
{
	// TODO: all of this
} XtAdpcmPatch;

// Representation of an XtInstrument in memory.
struct XtInstrument
{
	XtChannelType type;
	int16_t valid;
	char name[32];
	union
	{
		XtOpmPatch opm;
		// TODO: OPN, OPL support? Depends on platforms and/or exp hardware.
		XtAdpcmPatch adpcm;
	};
	// TODO: Vibrato / pitch mod params, etc.
} __attribute__((packed)) __attribute__((aligned(2)));
typedef struct XtInstrument XtInstrument;

struct XtInstrumentFileRecord
{
	uint8_t header_magic[8];  // "XTrackIn"
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