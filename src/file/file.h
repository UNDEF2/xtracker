#ifndef XT_FILE_H
#define XT_FILE_H

#include <stdio.h>
#include <stdint.h>

#include "xt/track.h"
#include "xt/types.h"

#define XT_WRITE_OR_ERROR(fd, buf, len) do							\
	{																\
		int _len = (len);											\
		if(_dos_write((fd), (void *)(buf), _len) != _len)			\
		{															\
			return XT_RES_FILE_WRITE_ERROR;							\
		}															\
	} while(0)

typedef uint32_t XtSegmentId;

typedef enum XtSegmentType
{
	XT_SEG_NULL,
	XT_SEG_PHRASE = 0x01000000,
	XT_SEG_INSTRUMENT = 0x02000000,
	XT_MAGIC = 0xFEEDFACE
} XtSegmentType;

static inline XtSegmentType xt_get_segment_type(XtSegmentId id)
{
	return id & 0xFF000000;
}

static inline XtSegmentType xt_get_segment_size(XtSegmentId id)
{
	return id & 0x00FFFFFF;
}

typedef struct XtSegmentHeader
{
} XtSegmentHeader;

typedef struct XtSegmentPhrase
{
	int16_t channel;
	int16_t frame;
	uint8_t payload[];
} XtSegmentPhrase;

typedef struct XtSegment
{
	XtSegmentId id;
	union
	{
		XtSegmentHeader header;
		XtSegmentPhrase phrase;
	};
} XtSegment;

#endif
