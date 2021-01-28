#include "xt_render.h"

#include "common.h"
#include <string.h>

#include "x68000/x68k_pcg.h"
#include "x68000/x68k_vidcon.h"

static const int kcolumn_width_tiles = 8;

static const uint16_t default_palette[] =
{
	// Line 1 - Notes, cursor
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0xFF, 0xFF, 0xFF),  // Note letters
	PAL_RGB8(0x60, 0x00, 0xFF),  // Cursor
	PAL_RGB8(0x50, 0x00, 0xEF),  // Cursor
	PAL_RGB8(0x40, 0x00, 0xDF),  // Cursor
	PAL_RGB8(0x30, 0x00, 0xCF),  // Cursor
	PAL_RGB8(0x20, 0x00, 0xBF),  // Cursor
	PAL_RGB8(0x10, 0x00, 0xAF),  // Cursor
	PAL_RGB8(0x00, 0x00, 0x9F),  // Cursor
	0, 0, 0, 0, 0, 0, 0,

	// Line 2 - Instrument, even row highlight
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x38, 0xFF, 0x89),
	PAL_RGB8(0x10, 0x10, 0x18),
	PAL_RGB8(0x10, 0x10, 0x18),
	PAL_RGB8(0x10, 0x10, 0x18),
	PAL_RGB8(0x10, 0x10, 0x18),
	PAL_RGB8(0x10, 0x10, 0x18),
	PAL_RGB8(0x10, 0x10, 0x18),
	PAL_RGB8(0x10, 0x10, 0x18),
	0, 0, 0, 0, 0, 0, 0,

	// Line 3 - Command, odd row highlight
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0xFF, 0x30, 0xE0),
	PAL_RGB8(0x20, 0x20, 0x28),
	PAL_RGB8(0x20, 0x20, 0x28),
	PAL_RGB8(0x20, 0x20, 0x28),
	PAL_RGB8(0x20, 0x20, 0x28),
	PAL_RGB8(0x20, 0x20, 0x28),
	PAL_RGB8(0x20, 0x20, 0x28),
	PAL_RGB8(0x20, 0x20, 0x28),
	0, 0, 0, 0, 0, 0, 0,

	// Line 4 - Param, line mark back
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x60, 0xFF, 0xFF),
	PAL_RGB8(0x20, 0x20, 0x38),
	PAL_RGB8(0x20, 0x20, 0x38),
	PAL_RGB8(0x20, 0x20, 0x38),
	PAL_RGB8(0x20, 0x20, 0x38),
	PAL_RGB8(0x20, 0x20, 0x38),
	PAL_RGB8(0x20, 0x20, 0x38),
	PAL_RGB8(0x20, 0x20, 0x38),
	0, 0, 0, 0, 0, 0, 0,

	// Line 5 - Empty, darkened cursor
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x40, 0x40, 0x40),
	PAL_RGB8(0x60, 0x00, 0x7F),  // Cursor
	PAL_RGB8(0x50, 0x00, 0x7F),  // Cursor
	PAL_RGB8(0x40, 0x00, 0x7F),  // Cursor
	PAL_RGB8(0x30, 0x00, 0x7F),  // Cursor
	PAL_RGB8(0x20, 0x00, 0x7F),  // Cursor
	PAL_RGB8(0x10, 0x00, 0x7F),  // Cursor
	PAL_RGB8(0x00, 0x00, 0x7F),  // Cursor
	0, 0, 0, 0, 0, 0, 0, 0,

	// Line 6 - line mark front
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x30, 0x30, 0x58),
	PAL_RGB8(0x30, 0x30, 0x58),
	PAL_RGB8(0x30, 0x30, 0x58),
	PAL_RGB8(0x30, 0x30, 0x58),
	PAL_RGB8(0x30, 0x30, 0x58),
	PAL_RGB8(0x30, 0x30, 0x58),
	PAL_RGB8(0x30, 0x30, 0x58),

	0, 0, 0, 0, 0, 0, 0,
};

// These strings look odd because they align to tiles in letter positions
// for non-letter things (e.g. '-'. '#').
// h = '#'
// i = '=' (left)
// j = '^' (left)
// m = '^' (right)
// k = '-' (left)
// l = '-' (right)
// n = '=' (right)

// Labels starting at XT_NOTE_NONE.
static const uint8_t note_labels[] =
{
	"--"  // 0x00 no note
	"C#"  // 0x01 CS
	"D-"  // 0x02 D
	"D#"  // 0x03 DS
	"XX"  // 0x04 
	"E-"  // 0x05 E
	"F-"  // 0x06 F
	"F#"  // 0x07 FS
	"XX"  // 0x08
	"G-"  // 0x09 G
	"G#"  // 0x0A GS
	"A-"  // 0x0B A
	"XX"  // 0x0C
	"A#"  // 0x0D AS
	"B-"  // 0x0E B
	"C-"  // 0x0F C
};

static void draw_backing(int16_t back_spacing, int16_t front_spacing)
{
	static const int16_t nt_height_cells = 512 / 8;
	static const int16_t nt_width_cells = 512 / 8;
	volatile uint16_t *nt1 = (volatile uint16_t *)PCG_BG1_NAME;

	static const uint8_t even_pal = 2;
	static const uint8_t odd_pal = 3;
	static const uint8_t back_pal = 4;
	static const uint8_t front_pal = 6;

	int16_t acc[2] = {back_spacing, front_spacing};

	for (int y = 0; y < nt_height_cells; y++)
	{
		uint8_t pal = 0;
		if (acc[0] == back_spacing)
		{
			acc[0] = 0;
			pal = back_pal;
		}
		else
		{
			acc[0]++;
		}
		if (acc[1] == front_spacing)
		{
			acc[1] = 1;
			pal = front_pal;
		}
		else
		{
			acc[1]++;
		}

		if (pal == 0)
		{
			pal = y % 2 ? odd_pal : even_pal;
		}
		for (int x = 0; x < nt_width_cells / kcolumn_width_tiles; x++)
		{
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2E);
			*nt1++ = PCG_ATTR(0, 0, pal, 0x2F);
		}
	}
}

static void draw_fm_column(XtPhrase *phrase, uint16_t x, uint16_t height)
{
	static const uint8_t note_pal = 1;
	static const uint8_t instr_pal = 2;
	static const uint8_t cmd_pal = 3;
	static const uint8_t arg_pal = 4;
	static const uint8_t empty_pal = 5;
	static const int16_t nt_width_cells = 512 / 8;
	static const int16_t cell_width_cells = kcolumn_width_tiles;
	const XtCell *cell = &phrase->cells[0];
	volatile uint16_t *nt0 = (volatile uint16_t *)PCG_BG0_NAME;
	nt0 += x;
	for (uint16_t i = 0; i < height; i++)
	{
		// Note and instrument.
		if (cell->note == XT_NOTE_OFF )
		{
			*nt0++ = PCG_ATTR(0, 0, note_pal, '=');
			*nt0++ = PCG_ATTR(0, 0, note_pal, '=');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
		}
		else if (cell->note == XT_NOTE_CUT)
		{
			*nt0++ = PCG_ATTR(0, 0, note_pal, '^');
			*nt0++ = PCG_ATTR(0, 0, note_pal, '^');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
		}
		else if (cell->note == XT_NOTE_NONE)
		{
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
		}
		else
		{
			// Note
			const uint8_t note = cell->note & XT_NOTE_TONE_MASK;
			*nt0++ = PCG_ATTR(0, 0, note_pal, note_labels[note * 2]);
			*nt0++ = PCG_ATTR(0, 0, note_pal, note_labels[1 + note * 2]);

			// Octave
			const uint8_t octave = cell->note >> 4;
			*nt0++ = PCG_ATTR(0, 0, note_pal, 0x10 + octave);

			// Instrument
			const uint8_t instr_high = 0x10 + (cell->inst >> 4);
			const uint8_t instr_low = 0x10 + (cell->inst & 0x0F);
			*nt0++ = PCG_ATTR(0, 0, instr_pal, instr_high);
			*nt0++ = PCG_ATTR(0, 0, instr_pal, instr_low);
		}

		// Commands and Params.
		for (uint16_t j = 0; j < ARRAYSIZE(cell->cmd); j++)
		{
			if (cell->cmd[j].cmd == XT_CMD_NONE)
			{
				*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
				*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
				*nt0++ = PCG_ATTR(0, 0, empty_pal, '-');
			}
			else
			{
				const uint8_t arg_high = 0x10 + (cell->cmd[j].arg >> 4);
				const uint8_t arg_low = 0x10 + (cell->cmd[j].arg & 0xF);
				*nt0++ = PCG_ATTR(0, 0, cmd_pal, cell->cmd[j].cmd);
				*nt0++ = PCG_ATTR(0, 0, arg_pal, arg_high);
				*nt0++ = PCG_ATTR(0, 0, arg_pal, arg_low);
			}
		}
		cell++;
		nt0 += (nt_width_cells - cell_width_cells);
	}
}

void xt_track_renderer_init(XtTrackRenderer *r)
{
	memset(r, 0, sizeof(*r));
	for (int i = 0; i < ARRAYSIZE(r->last_phrase_num); i++)
	{
		r->last_phrase_num[i] = 0xFFFF;
	}
	for (int i = 0; i < ARRAYSIZE(r->channel_dirty); i++)
	{
		r->channel_dirty[i] = 1;
	}

	// Load the palette.
	for (int i = 0; i < ARRAYSIZE(default_palette); i++)
	{
		x68k_vidcon_set_pcg_color(0x10 + i, default_palette[i]);
	}
}

void xt_track_renderer_repaint_channel(XtTrackRenderer *r, uint16_t channel)
{
	r->channel_dirty[channel] = 1;
}

void xt_track_renderer_tick(XtTrackRenderer *r, Xt *xt, uint16_t frame)
{
	int phrase_x_pos = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(r->channel_dirty); i++)
	{
		const uint16_t phrase_id = xt_track_get_phrase_number_for_frame(&xt->track, i, frame);

		if (phrase_id != r->last_phrase_num[i])
		{
			r->last_phrase_num[i] = phrase_id;
			r->channel_dirty[i] = 1;
		}

		if (r->channel_dirty[i])
		{
			r->channel_dirty[i] = 0;
			if (i < 8)
			{
				draw_fm_column(&xt->track.phrases[phrase_id],
				                  phrase_x_pos, xt->track.phrase_length);
			}
			else
			{
				// TODO: Draw PCM phrases (easy...)
			}
		}

		// Space note channels separately from the PCM ones
		if (i < 8) phrase_x_pos += kcolumn_width_tiles;
		else phrase_x_pos += 1;
	}

	// Repaint the background if needed.
	if (r->row_highlight[0] != xt->config.row_highlight[0] ||
	    r->row_highlight[1] != xt->config.row_highlight[1])
	{
		r->row_highlight[0] = xt->config.row_highlight[0];
		r->row_highlight[1] = xt->config.row_highlight[1];
		draw_backing(r->row_highlight[0], r->row_highlight[1]);
	}
}
