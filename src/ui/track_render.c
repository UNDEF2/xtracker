#include "ui/track_render.h"

#include "common.h"
#include <string.h>
#include <stdio.h>

#include "xbase/crtc.h"
#include "xbase/vidcon.h"
#include "xbase/memmap.h"

#include "cgprint.h"

// Labels starting at XT_NOTE_NONE.
static const char s_note_strings[][3] =
{
	"  ",
	"C ",
	"C#",
	"D ",
	"D#",
	"E ",
	"F ",
	"F#",
	"G ",
	"G#",
	"A ",
	"A#",
	"B ",
};

static const char s_octave_strings[8][2] =
{
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
};

static const char s_hex_strings[16][2] =
{
	"0", "1", "2", "3",
	"4", "5", "6", "7",
	"8", "9", "A", "B",
	"C", "D", "E", "F"
};

static void draw_empty_column(uint16_t x, uint16_t height,
                              int16_t back_spacing, int16_t front_spacing)
{
	for (uint16_t i = 0; i < height; i++)
	{
		const uint16_t draw_x = x * XT_RENDER_CELL_W_PIXELS;
		const uint16_t draw_y = i * XT_RENDER_CELL_H_PIXELS;
		cgbox(XT_RENDER_CELL_PLANE, 0,
		      draw_x, draw_y,
		      draw_x + (XT_RENDER_CELL_W_PIXELS * XT_RENDER_CELL_CHARS),
		      draw_y + (XT_RENDER_CELL_H_PIXELS));
	}
}

static void draw_opm_column(const XtPhrase *phrase, uint16_t x, uint16_t height,
                            int16_t back_spacing, int16_t front_spacing)
{
	static const uint8_t kpal_inactive = 1;
	static const uint8_t kpal_note = 2;
	static const uint8_t kpal_octave = 3;
	static const uint8_t kpal_inst = 4;
	static const uint8_t kpal_cmd = 5;
	static const uint8_t kpal_arg;
	

	const XtCell *cell = &phrase->cells[0];
	for (uint16_t i = 0; i < height; i++)
	{
		uint16_t draw_x = x * XT_RENDER_CELL_W_PIXELS;
		const uint16_t draw_y = i * XT_RENDER_CELL_H_PIXELS;


		// Note and instrument.
		if (cell->note == XT_NOTE_OFF )
		{
			cgprint(XT_RENDER_CELL_PLANE, kpal_inactive | CG_ATTR_OPAQUE,
			        "== -- ", draw_x, draw_y);
			draw_x += 6 * XT_RENDER_CELL_W_PIXELS;
		}
		else if (cell->note == XT_NOTE_CUT)
		{
			cgprint(XT_RENDER_CELL_PLANE, kpal_inactive | CG_ATTR_OPAQUE,
			        "^^ -- ", draw_x, draw_y);
			draw_x += 6 * XT_RENDER_CELL_W_PIXELS;
		}
		else if (cell->note == XT_NOTE_NONE)
		{
			cgprint(XT_RENDER_CELL_PLANE, kpal_inactive | CG_ATTR_OPAQUE,
			        "-- -- ", draw_x, draw_y);
			draw_x += 6 * XT_RENDER_CELL_W_PIXELS;
		}
		else
		{
			// Note
			const uint8_t note = cell->note & XT_NOTE_TONE_MASK;
			cgprint(XT_RENDER_CELL_PLANE, kpal_note | CG_ATTR_OPAQUE,
			        s_note_strings[note], draw_x, draw_y);
			draw_x += 2 * XT_RENDER_CELL_W_PIXELS;

			// Octave
			const uint8_t octave = cell->note >> 4;
			cgprint(XT_RENDER_CELL_PLANE, kpal_octave | CG_ATTR_OPAQUE,
			        s_octave_strings[octave], draw_x, draw_y);
			draw_x += 1 * XT_RENDER_CELL_W_PIXELS;

			// Instrument
			const uint8_t instr_high = (cell->inst >> 4);
			const uint8_t instr_low = (cell->inst & 0x0F);
			cgprint(XT_RENDER_CELL_PLANE, kpal_inst | CG_ATTR_OPAQUE,
			        s_hex_strings[instr_high], draw_x, draw_y);
			draw_x += 1 * XT_RENDER_CELL_W_PIXELS;
			cgprint(XT_RENDER_CELL_PLANE, kpal_inst | CG_ATTR_OPAQUE,
			        s_hex_strings[instr_low], draw_x, draw_y);
			draw_x += 2 * XT_RENDER_CELL_W_PIXELS;
		}

		// Commands and Params.
		for (uint16_t j = 0; j < ARRAYSIZE(cell->cmd); j++)
		{
			if (cell->cmd[j].cmd == XT_CMD_NONE)
			{
				cgprint(XT_RENDER_CELL_PLANE, kpal_inactive | CG_ATTR_OPAQUE,
				        "---", draw_x, draw_y);
				draw_x += 3 * XT_RENDER_CELL_W_PIXELS;
			}
			else
			{
				const uint8_t arg_high = (cell->cmd[j].arg >> 4);
				const uint8_t arg_low = (cell->cmd[j].arg & 0xF);
				char cmdstr[3];
				cmdstr[0] = cell->cmd[j].cmd;
				cmdstr[1] = '\0';
				cgprint(XT_RENDER_CELL_PLANE, kpal_cmd | CG_ATTR_OPAQUE,
				        cmdstr, draw_x, draw_y);
				draw_x += 1 * XT_RENDER_CELL_W_PIXELS;
				cmdstr[0] = s_hex_strings[arg_high][0];
				cmdstr[1] = s_hex_strings[arg_low][1];
				cmdstr[2] = '\0';
				cgprint(XT_RENDER_CELL_PLANE, kpal_arg | CG_ATTR_OPAQUE,
				        cmdstr, draw_x, draw_y);
				draw_x += 2 * XT_RENDER_CELL_W_PIXELS;
			}
		}
		cell++;
	}
}

void xt_track_renderer_init(XtTrackRenderer *r)
{
	memset(r, 0, sizeof(*r));
	for (int i = 0; i < ARRAYSIZE(r->chan); i++)
	{
		r->chan[i].last_phrase = NULL;
		r->chan[i].dirty = true;
	}

	r->visible_channels = 8;
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
//	xb_crtc_set_gp1_xscroll(r->cam_x);
//	xb_crtc_set_gp1_yscroll(r->cam_y);
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
