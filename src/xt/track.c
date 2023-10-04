#include "xt/track.h"
#include <dos.h>
#include <string.h>
#include "common.h"

XtPhrase *xt_track_get_phrase(XtTrack *t, uint16_t channel, uint16_t frame)
{
	const uint16_t idx = t->frames[frame].phrase_id[channel];
	return &t->channel_data[channel].phrases[idx];
}

void xt_track_init(XtTrack *t)
{
	// Set defaults.
	memset(t, 0, sizeof(*t));
	t->num_frames = 1;
	t->num_instruments = 1;
	t->row_highlight[0] = 4;
	t->row_highlight[1] = 16;

	t->ticks_per_row = 6;
	t->timer_period = 0x30;

	t->phrase_length = 32;
	t->loop_point = 0;

	for (int16_t i = 0; i < ARRAYSIZE(t->channel_data); i++)
	{
		XtTrackChannelData *data = &t->channel_data[i];
		if (i < 8)
		{
			data->type = XT_CHANNEL_OPM;
			data->voice_number = i;
		}
		else
		{
			data->type = XT_CHANNEL_ADPCM;
			data->voice_number = i - 8;
		}
	}
}

XtResult xt_track_save_to_file(const XtTrack *t, const char *fname)
{
	int fname_handle = _dos_open(fname, 0x0001);
	if (fname_handle < 0)
	{
		printf("Creating new file \"%s\".\n", fname);
		fname_handle = _dos_create(fname, 0);  // __a_ ____
		if (fname_handle < 0)
		{
			printf("Could not create file \"%s\" (code %d)\n",
			       fname, fname_handle);
			return XT_RES_FILE_WRITE_ERROR;
		}
	}

	const int bytes_written = _dos_write(fname_handle,
	                                     (const char *)t,
	                                     sizeof(*t));
	printf("Wrote %d bytes to \"%s\".\n", bytes_written, fname);

	_dos_close(fname_handle);
	return XT_RES_OK;
}

XtResult xt_track_load_from_file(XtTrack *t, const char *fname)
{
	const int fname_handle = _dos_open(fname, 0x0000);
	// -2: file does not exist
	if (fname_handle < 0)
	{
		printf("Could not open \"%s\".\n", fname);
		return XT_RES_FILE_NOT_FOUND;
	}

	const int bytes_read = _dos_read(fname_handle, (char *)t, sizeof(*t));

	printf("Read %d bytes from \"%s\".\n", bytes_read, fname);

	_dos_close(fname_handle);

	return XT_RES_OK;
}
