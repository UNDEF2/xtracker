#include "xt/instrument.h"

#include "core/macro.h"

#include <string.h>

XtResult load_non_native_instrument(FILE *f, XtInstrument *out)
{
	XtResult ret = XT_RES_OK;

	// TODO: Support formats - VGI, TFI, DFM, OPM, etc.

	ret = XT_RES_UNSUPPORTED;

	return ret;
}
