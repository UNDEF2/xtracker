#ifndef XT_INSTRUMENT_OPN_FM_H
#define XT_INSTRUMENT_OPN_FM_H

#include <stdint.h>

#define XT_OPN_OP_COUNT 4

typedef struct XtInstrumentOpnFm
{
	uint8_t con;  // $B0 bits 0-2
	uint8_t fl;   // $B0 bits 3-5
	uint8_t pms;  // $B4 bits 0-2
	uint8_t ams;  // $B4 bits 4-5
	union  // $50 bits 0-4
	{
		uint8_t ar[XT_OPN_OP_COUNT];
		uint32_t ar_u32;
	};
	union  // $60 bits 0-5
	{
		uint8_t d1r[XT_OPN_OP_COUNT];
		uint32_t d1r_u32;
	};
	union  // $70 bits 0-4
	{
		uint8_t d2r[XT_OPN_OP_COUNT];
		uint32_t d2r_u32;
	};
	union  // $80 bits 0-3
	{
		uint8_t rr[XT_OPN_OP_COUNT];
		uint32_t rr_u32;
	};
	union  // $80 bits 0-3
	{
		uint8_t d1l[XT_OPN_OP_COUNT];
		uint32_t d1l_u32;
	};
	union  // $40 bits 0-6
	{
		uint8_t tl[XT_OPN_OP_COUNT];
		uint32_t tl_u32;
	};
	union  // $50 bits 6-7
	{
		uint8_t ks[XT_OPN_OP_COUNT];
		uint32_t ks_u32;
	};
	union  // $30 bits 0-3
	{
		uint8_t mul[XT_OPN_OP_COUNT];
		uint32_t mul_u32;
	};
	union  // $30 bits 406
	{
		// TODO: use int8_t once a signed entry interface is in
		uint8_t dt1[XT_OPN_OP_COUNT];
		uint32_t dt1_u32;
	};
	union  // $60 bit 7
	{
		uint8_t ame[XT_OPN_OP_COUNT];
		uint32_t ame_u32;
	};
} XtInstrumentOpnFm;

#endif  // XT_INSTRUMENT_OPN_FM_H
