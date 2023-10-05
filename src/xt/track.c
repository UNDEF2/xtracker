#include "xt/track.h"
#include <dos.h>
#include <limits.h>
#include <string.h>
#include "core/macro.h"
#include "file/file.h"

// TODO: is this size appropriate?
static union
{
	char b[1024];
	XtSegment s;
} segment_buf;

XtResult xt_phrase_save_to_file(int16_t channel, int16_t frame,
								const XtPhrase *p, int fd);

XtPhrase *xt_track_get_phrase(XtTrack *t, uint16_t channel,
							  uint16_t frame)
{
	const uint16_t idx = t->frames[frame].phrase_id[channel];
	return &t->channel_data[channel].phrases[idx];
}

const XtPhrase *xt_track_get_phrase_const(const XtTrack *t, uint16_t channel,
										  uint16_t frame)
{
	const uint16_t idx = t->frames[frame].phrase_id[channel];
	return &t->channel_data[channel].phrases[idx];
}

void xt_track_init(XtTrack *t)
{
	// Set defaults.
	memset(t, 0, sizeof(*t));

	t->meta.row_highlight[0] = 4;
	t->meta.row_highlight[1] = 16;
	t->meta.phrase_length = 32;
	t->meta.timer_period = 0x40;
	t->meta.groove[0] = 6;
	t->meta.groove[1] = 6;
	t->meta.loop_point = 0;

	t->num_instruments = 1;
	t->num_frames = 1;

	for (int16_t i = 0; i < ARRAYSIZE(t->channel_data); i++)
	{
		XtTrackChannelData *data = &t->channel_data[i];
		if (i < 8)
		{
			data->type = XT_INSTRUMENT_TYPE_OPM;
			data->voice_number = i;
		}
		else
		{
			data->type = XT_INSTRUMENT_TYPE_MSM6258;
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

	// traverse the module
	uint32_t magic = XT_MAGIC;
	XT_WRITE_OR_ERROR(fname_handle, &magic, sizeof(magic));
	
	for (int16_t frame = 0; frame < t->num_frames; frame++)
	{
		for (int16_t channel = 0; channel < (int16_t)ARRAYSIZE(t->channel_data);
			channel++)
		{
			const XtPhrase *phrase = xt_track_get_phrase_const(t, channel,
															   frame);
			XtResult res = xt_phrase_save_to_file(channel, frame, phrase,
												  fname_handle);
			if (res != XT_RES_OK) {
				_dos_close(fname_handle);
				return res;
			}
		}
	}

	_dos_close(fname_handle);
	return XT_RES_OK;
}

XtResult xt_phrase_save_to_file(int16_t channel, int16_t frame,
								const XtPhrase *p, int fd)
{
	XtSegment *const seg = &segment_buf.s;
	XtSegmentPhrase *phrase = &seg->phrase;
	phrase->channel = channel;
	phrase->frame = frame;
	uint8_t *phrase_pos = &phrase->payload[0];
	uint8_t skip = 0;
	for (int16_t i = 0; i < XT_PHRASE_MAX_ROWS; i++)
	{
		const XtCell *cell = &p->cells[i];
		uint8_t mask = 0;
		//  we'll be back later if the mask was nonzero
		uint8_t *cell_pos = phrase_pos + 2;
		
		if (cell->note != XT_NOTE_NONE)
		{
			*cell_pos++ = cell->note;
			mask |= 0x80;
		}
		if (cell->inst)
		{
			*cell_pos++ = cell->inst;
			mask |= 0x40;
		}
		if (cell->vol&0x7F)
		{
			*cell_pos++ = cell->vol;
			mask |= 0x20;
		}

		for (int16_t i = 0; i < XT_CMD_COL_COUNT; i++)
		{
			XtCmd cmd = cell->cmd[i].cmd;
			if (cmd != XT_CMD_NONE)
			{
				*cell_pos++ = cmd;
				*cell_pos++ = cell->cmd[i].arg;
				mask |= 1U << (CHAR_BIT - 3 - 1 - i);
			}
		}

		// Will need special case if XT_PHRASE_MAX_ROWS > 255, but ok for now
		if (mask == 0)
		{
			skip++;
		}
		else
		{
			phrase_pos[0] = skip;
			phrase_pos[1] = mask;
			phrase_pos = cell_pos;
		}
	}
	// frame was empty
	if (skip == XT_PHRASE_MAX_ROWS)
	{
		return XT_RES_OK;
	}

    size_t size = phrase_pos - (uint8_t *)seg;
	seg->id = XT_SEG_PHRASE | size;
	printf("DEBUG: %lx\n", seg->id);

	// if the size is odd, we need to pad
	size_t size_written = size + (size & 1);
	if (size_written != size) *phrase_pos = 0; // ensure padding is known

	XT_WRITE_OR_ERROR(fd, seg, size_written);
	
	return XT_RES_OK;
}

XtResult xt_phrase_load_from_file(XtTrack *t, const XtSegment **seg) {
	const XtSegmentPhrase *pseg = &((*seg)->phrase);
	printf("DEBUG: loading channel %d, frame %d\n", pseg->channel, pseg->frame);
    XtPhrase *phrase = xt_track_get_phrase(t, pseg->channel, pseg->frame);

	const size_t size = xt_get_segment_size((*seg)->id);
	const XtSegment *end = (const XtSegment *)((char *)*seg + size);

	const uint8_t *work = pseg->payload;
	XtCell *cell = phrase->cells;
	do
	{
		cell += *work++;
		uint8_t mask = *work++;
		if (mask & 0x80)
		{
			cell->note = *work++;
		}
		if (mask & 0x40)
		{
			cell->inst = *work++;
		}
		if (mask & 0x20)
		{
			cell->vol = *work++;
		}

		mask <<= 3;
		uint16_t i = 0;
		while (mask)
		{
			if (mask & 0x80)
			{
				cell->cmd[i].cmd = *work++;
				cell->cmd[i].arg = *work++;
			}
			i++;
			mask <<= 1;
		}
		cell++;
	} while (work < (const uint8_t *)end);
	
	*seg = (const XtSegment *)((const char *)end + (size & 1));
	return XT_RES_OK;
}

XtResult xt_track_load_from_file(XtTrack *t, const char *fname)
{
	static union
	{
		char buf[121072];
		XtSegment seg;
	} u;
	const int fname_handle = _dos_open(fname, 0x0000);
	// -2: file does not exist
	if (fname_handle < 0)
	{
		printf("Could not open \"%s\".\n", fname);
		return XT_RES_FILE_NOT_FOUND;
	}

	const int bytes_read = _dos_read(fname_handle, u.buf, sizeof(u.buf));

	printf("Read %d bytes from \"%s\".\n", bytes_read, fname);

	_dos_close(fname_handle);

	if (u.seg.id != XT_MAGIC)
	{
		printf("Invalid magic number 0x%04lX.\n", u.seg.id);
		return XT_RES_BAD_FORMAT;
	}

	const XtSegment *seg = (const XtSegment *)(u.buf + sizeof(XtSegmentId));
	const XtSegment *end = (const XtSegment *)&u.buf[bytes_read];
	XtResult res;
	while (seg < end)
	{
		XtSegmentType type = xt_get_segment_type(seg->id);
		// TODO: fix indent of case labels
		switch (type)
		{
		case XT_SEG_NULL:
			// optional terminator
			return XT_RES_OK;
		case XT_SEG_PHRASE:
			res = xt_phrase_load_from_file(t, &seg);
			break;
		default:
			printf("Unsupported segment type: 0x%04x.\n", type);
			return XT_RES_UNSUPPORTED;
		}
		if (res != XT_RES_OK) return res;
	}
	return XT_RES_OK;
}
