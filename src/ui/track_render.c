#include "ui/track_render.h"

#include "common.h"
#include <string.h>
#include <stdio.h>

#include "xbase/pcg.h"
#include "xbase/vidcon.h"
#include "xbase/memmap.h"

// Labels starting at XT_NOTE_NONE.
static const uint8_t s_note_label_mappings[13][3] =
{
	{0x08, 0x0F, 0x00},  // 0x00 no note
	{0x72, 0x7B, 0x00},  // 0x01 C
	{0x72, 0x7C, 0x00},  // 0x02 C#
	{0x73, 0x7B, 0x00},  // 0x03 D
	{0x73, 0x7C, 0x00},  // 0x04 D#
	{0x74, 0x7B, 0x00},  // 0x05 E
	{0x75, 0x7B, 0x00},  // 0x06 F
	{0x75, 0x7C, 0x00},  // 0x07 F#
	{0x76, 0x7B, 0x00},  // 0x08 G
	{0x76, 0x7C, 0x00},  // 0x09 G#
	{0x70, 0x7B, 0x00},  // 0x0A A
	{0x70, 0x7C, 0x00},  // 0x0B A#
	{0x71, 0x7B, 0x00},  // 0x0C B
};

static void draw_empty_column(uint16_t x, uint16_t height,
                              int16_t back_spacing, int16_t front_spacing)
{
	int16_t acc[2] = {0, 0};

	volatile uint16_t *nt0 = (volatile uint16_t *)XB_PCG_BG0_NAME;
	nt0 += (x % XT_RENDER_NT_CHARS);

	static const uint8_t normal_pal = 1;
	static const uint8_t back_pal = 2;
	static const uint8_t front_pal = 3;
	static const int16_t nt_width_cells = 512 / 8;
	static const int16_t cell_width_cells = XT_RENDER_CELL_CHARS;
	for (uint16_t i = 0; i < height; i++)
	{
		uint8_t pal = normal_pal;
		if (acc[0] <= 0)
		{
			pal = back_pal;
			acc[0] = back_spacing;
		}
		acc[0]--;
		if (acc[1] <= 0)
		{
			pal = front_pal;
			acc[1] = front_spacing;
		}
		acc[1]--;

		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x78);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);

		nt0 += (nt_width_cells - cell_width_cells);
	}
}

static void draw_opm_column(const XtPhrase *phrase, uint16_t x, uint16_t height,
                            int16_t back_spacing, int16_t front_spacing)
{
	int16_t acc[2] = {0, 0};
	static const uint8_t normal_pal = 1;
	static const uint8_t back_pal = 2;
	static const uint8_t front_pal = 3;
	static const int16_t nt_width_cells = 512 / 8;
	static const int16_t cell_width_cells = XT_RENDER_CELL_CHARS;
	const XtCell *cell = &phrase->cells[0];
	volatile uint16_t *nt0 = (volatile uint16_t *)XB_PCG_BG0_NAME;
	nt0 += (x % XT_RENDER_NT_CHARS);
	for (uint16_t i = 0; i < height; i++)
	{
		uint8_t pal = normal_pal;
		if (acc[0] <= 0)
		{
			pal = back_pal;
			acc[0] = back_spacing;
		}
		acc[0]--;
		if (acc[1] <= 0)
		{
			pal = front_pal;
			acc[1] = front_spacing;
		}
		acc[1]--;

		// Note and instrument.
		if (cell->note == XT_NOTE_OFF )
		{
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x79);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7D);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		}
		else if (cell->note == XT_NOTE_CUT)
		{
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7A);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7E);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		}
		else if (cell->note == XT_NOTE_NONE)
		{
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x78);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
		}
		else
		{
			// Note
			const uint8_t note = cell->note & XT_NOTE_TONE_MASK;
			*nt0++ = XB_PCG_ATTR(0, 0, pal, s_note_label_mappings[note][0]);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, s_note_label_mappings[note][1]);

			// Octave
			const uint8_t octave = cell->note >> 4;
			*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x10 + octave);

			// Instrument
			const uint8_t instr_high = 0x20 + (cell->inst >> 4);
			const uint8_t instr_low = 0x20 + (cell->inst & 0x0F);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, instr_high);
			*nt0++ = XB_PCG_ATTR(0, 0, pal, instr_low);
		}

		// Commands and Params.
		for (uint16_t j = 0; j < ARRAYSIZE(cell->cmd); j++)
		{
			if (cell->cmd[j].cmd == XT_CMD_NONE)
			{
				*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
				*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
				*nt0++ = XB_PCG_ATTR(0, 0, pal, 0x7F);
			}
			else
			{
				const uint8_t arg_high = 0x60 + (cell->cmd[j].arg >> 4);
				const uint8_t arg_low = 0x60 + (cell->cmd[j].arg & 0xF);
				*nt0++ = XB_PCG_ATTR(0, 0, pal, cell->cmd[j].cmd);
				*nt0++ = XB_PCG_ATTR(0, 0, pal, arg_high);
				*nt0++ = XB_PCG_ATTR(0, 0, pal, arg_low);
			}
		}
		cell++;
		nt0 += (nt_width_cells - cell_width_cells);
	}
}

void xt_track_renderer_init(XtTrackRenderer *r)
{
	// State init
	memset(r, 0, sizeof(*r));
	for (int i = 0; i < ARRAYSIZE(r->chan); i++)
	{
		r->chan[i].last_phrase = NULL;
		r->chan[i].dirty = true;
	}

	r->visible_channels = XT_RENDER_VISIBLE_CHANNELS;

	// Load PCG data
	FILE *f;
	f = fopen("RES\\PCG.BIN", "rb");
	if (!f)
	{
		fprintf(stderr, "Error: Could not load PCG data.\n");
	}
	else
	{
		fseek(f, 0, SEEK_END);
		const size_t pcg_size = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread((uint8_t *)XB_PCG_TILE_DATA, 1, pcg_size, f);
		fclose(f);
	}
}

void xt_track_renderer_repaint_channel(XtTrackRenderer *r, uint16_t channel)
{
	r->chan[channel].dirty = true;
}

void xt_track_renderer_tick(XtTrackRenderer *r, Xt *xt, uint16_t frame)
{
	_Static_assert(ARRAYSIZE(r->chan) == ARRAYSIZE(xt->chan));

	int draw_x = 0;

	const int highlight_changed = (r->row_highlight[0] != xt->config.row_highlight[0] ||
	                               r->row_highlight[1] != xt->config.row_highlight[1]);

	if (highlight_changed)
	{
		r->row_highlight[0] = xt->config.row_highlight[0];
		r->row_highlight[1] = xt->config.row_highlight[1];
	}

	for (int16_t i = 0; i < ARRAYSIZE(r->chan); i++)
	{
		XtChannelRenderState *chan = &r->chan[i];
		const XtPhrase *phrase = xt_track_get_phrase(&xt->track, i, frame);

		// Check if a channel's phrase ID has changed, and mark the channel as
		// dirty if so.
		if (phrase != chan->last_phrase)
		{
			chan->last_phrase = phrase;
			chan->dirty = true;
		}

		if (highlight_changed)
		{
			chan->dirty = true;
		}

		// If the channel is dirty, and is "on-plane", repaint it.
		if (chan->dirty && chan->active)
		{
			chan->dirty = false;

			const int16_t hl[2] = {r->row_highlight[0], r->row_highlight[1]};
			const int16_t len = xt->track.phrase_length;

			switch (xt->chan[i].type)
			{
				default:
					draw_empty_column(draw_x, len, hl[0], hl[1]);
					break;

				case XT_CHANNEL_OPM:
					draw_opm_column(phrase, draw_x, len, hl[0], hl[1]);

					break;
				case XT_CHANNEL_ADPCM:
					// TODO
					draw_empty_column(draw_x, len, hl[0], hl[1]);
					break;
			}
		}

		draw_x += XT_RENDER_CELL_CHARS;
	}

	// Move the planes around for the camera.
	xb_pcg_set_bg0_xscroll(r->cam_x);
	xb_pcg_set_bg0_yscroll(r->cam_y);
}

void xt_track_renderer_set_camera(XtTrackRenderer *r, int16_t x, int16_t y)
{
	r->cam_x = x;
	r->cam_y = y;

	const int16_t left_visible_channel = (x) /
	                                     (XT_RENDER_CELL_W_PIXELS *
	                                      XT_RENDER_CELL_CHARS);
	const int16_t right_visible_channel = left_visible_channel + r->visible_channels;

	for (int16_t i = 0; i < ARRAYSIZE(r->chan); i++)
	{
		const int16_t prev = r->chan[i].active;
		const bool new = (i >= left_visible_channel && i < right_visible_channel);
		r->chan[i].active = new;
		if (new && prev != new)
		{
			r->chan[i].dirty = true;
		}
	}
}
