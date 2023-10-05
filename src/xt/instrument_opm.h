#ifndef XT_INSTRUMENT_OPM_H
#define XT_INSTRUMENT_OPM_H

#include "xbase/opm.h"
#include <stdint.h>

typedef struct XtInstrumentOpm
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
		// TODO: use int8_t once a signed entry interface is in
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
} XtInstrumentOpm;

#endif  // XT_INSTRUMENT_OPM_H
