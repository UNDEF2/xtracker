#include "xt/instrument.h"

#include "core/macro.h"

#include <string.h>

static const char kheader_magic[] = {'X', 'T', 'r', 'a', 'c', 'k', 'I', 'n'};

XtResult load_non_native_instrument(FILE *f, XtInstrument *out)
{
	XtResult ret = XT_RES_OK;

	// TODO: Support formats - VGI, TFI, DFM, OPM, etc.

	ret = XT_RES_UNSUPPORTED;

	return ret;
}

XtResult xt_instrument_load_from_file(FILE *f, XtInstrument *out)
{
	if (!f) return XT_RES_BAD_HANDLE;
	memset(out, 0, sizeof(*out));

	// Check for native instrument header.
	for (int i = 0; i < ARRAYSIZE(kheader_magic); i++)
	{
		const int fetch = fgetc(f);
		// Not a native instrument file - try loading as another.
		if (fetch != kheader_magic[i])
		{
			fseek(f, 0, SEEK_SET);
			return load_non_native_instrument(f, out);
		}
	}

	// Read native instrument data.
	const int read = fread(out, sizeof(*out), 1, f);
	if (read < sizeof(*out))
	{
		const XtResult ret = feof(f) ? XT_RES_FILE_TRUNCATED : XT_RES_FILE_READ_ERROR;
		return ret;
	}

	return XT_RES_OK;
}

XtResult xt_instrument_write_to_file(const XtInstrument *in, FILE *f)
{
	// Write header.
	for (int i = 0; i < ARRAYSIZE(kheader_magic); i++)
	{
		if (fputc(kheader_magic[i], f) == EOF)
		{
			return XT_RES_FILE_WRITE_ERROR;
		}
	}

	// Write native instrument data.
	const int written = fwrite(in, sizeof(*in), 1, f);
	if (written < sizeof(*in))
	{
		return XT_RES_FILE_WRITE_ERROR;
	}

	return XT_RES_OK;
}
