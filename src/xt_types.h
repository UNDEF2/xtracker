#ifndef XT_TYPES_H
#define XT_TYPES_H

#include <stdint.h>
#include "x68000/x68k_opm.h"

typedef enum XtResult
{
	XT_RES_OK,
	XT_RES_BAD_HANDLE,
	XT_RES_BAD_FORMAT,
	XT_RES_FILE_NOT_FOUND,
	XT_RES_FILE_TRUNCATED,
	XT_RES_FILE_READ_ERROR,
	XT_RES_FILE_WRITE_ERROR,
	XT_RES_UNSUPPORTED,
} XtResult;

#endif
