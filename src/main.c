#include <stdio.h>
#include <dos.h>
#include <iocs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "x68000/x68k_pcg.h"
#include "x68000/x68k_joy.h"
#include "x68000/x68k_crtc.h"
#include "x68000/x68k_vidcon.h"
#include "x68000/x68k_vbl.h"

#include "common.h"
#include "irq.h"
#include "xt.h"
#include "xt_render.h"
#include "xt_phrase_editor.h"
#include "xt_keys.h"
#include "xt_vbl.h"

static Xt xt;
static XtTrackRenderer renderer;
static XtKeys keys;
static XtPhraseEditor phrase_editor;

#define CRT_FLAGS 0x0001

typedef struct DisplayMode
{
	X68kCrtcConfig crtc;
	X68kPcgConfig pcg;
	X68kVidconConfig vidcon;
} DisplayMode;

// This is a pretty messed up 512x256 active picture mode that abuses the HRES
// bit of the PCG to get 8x8 tiles when in 512px width, but sprites end up
// broken as the right side of the sprite buffer is not cleared.
static const DisplayMode mode_512_256 =
{
	{
		0x004D, 0x0005, 0x000A, 0x0049,
		0x0103, 0x0002, 0x000F, 0x00FF,
		0x20, CRT_FLAGS
	},
	{
		0x00FF, 0x000E, 0x001D, 0x0000
	},
	{
		(CRT_FLAGS >> 8), 0x12E4, 0x007F
	}
};

static const DisplayMode mode_448 =
{
	{
		0x004B, 0x0003, 0x000A, 0x0042,
		0x0106, 0x0002, 0x0014, 0x0103,
		0x20, CRT_FLAGS
	},
	{
		0x00FF, 0x000E, 0x0014, 0x0000
	},
	{
		(CRT_FLAGS >> 8), 0x12E4, 0x007F
	}
};

int video_init(void)
{
	const DisplayMode *mode = &mode_448;
	x68k_crtc_init(&mode->crtc);
	x68k_pcg_init(&mode->pcg);
	x68k_vidcon_init(&mode->vidcon);

	xt_vbl_init();

	// Clear PCG nametables and data
	x68k_vbl_wait_for_vblank();
	x68k_pcg_set_disp_en(0);
	memset((void *)PCG_TILE_DATA, 0, 0x4000);
	memset((void *)PCG_SPR_TABLE, 0, 0x400);
	memset((void *)PCG_BG0_NAME, 0, 0x2000);
	memset((void *)PCG_BG1_NAME, 0, 0x2000);

	// Load font tileset
	FILE *f;
	f = fopen("PCG.BIN", "rb");
	if (!f)
	{
		fprintf(stderr, "Error: Could not load PCG data.\n");
	}
	volatile uint16_t *pcg_data = (volatile uint16_t *)PCG_TILE_DATA;
	while (!feof(f))
	{
		uint16_t word = 0;
		word |= (fgetc(f) << 8);
		word |= fgetc(f);
		*pcg_data++ = word;
	}
	fclose(f);

	// Set up PCG to use backgrounds for our text
	x68k_pcg_set_bg0_txsel(0);
	x68k_pcg_set_bg1_txsel(1);
	x68k_pcg_set_bg0_enable(1);
	x68k_pcg_set_bg1_enable(1);

	x68k_pcg_set_bg0_xscroll(0);
	x68k_pcg_set_bg1_xscroll(-4);

	x68k_pcg_set_bg0_yscroll(0);
	x68k_pcg_set_bg1_yscroll(0);

	x68k_pcg_set_disp_en(1);
	return 1;
}

void set_demo_instruments(void)
{
	XtInstrument *ins = &xt.track.instruments[0];

	// The bass from Private Eye (Daiginjou)
	ins->reg_20_pan_fl_con = 0xFB;
	ins->reg_38_pms_ams = 0;

	ins->reg_40_dt1_mul[0] = 8;
	ins->reg_60_tl[0] = 30;
	ins->reg_80_ks_ar[0] = 27;
	ins->reg_A0_ame_d1r[0] = 14;
	ins->reg_C0_dt2_d2r[0] = 0;
	ins->reg_E0_d1l_rr[0] = (3 << 4) | 10;

	ins->reg_40_dt1_mul[1] = 2;
	ins->reg_60_tl[1] = 45;
	ins->reg_80_ks_ar[1] = 31;
	ins->reg_A0_ame_d1r[1] = 12;
	ins->reg_C0_dt2_d2r[1] = 0;
	ins->reg_E0_d1l_rr[1] = (3 << 4) | 10;

	ins->reg_40_dt1_mul[2] = 0;
	ins->reg_60_tl[2] = 15;
	ins->reg_80_ks_ar[2] = 31;
	ins->reg_A0_ame_d1r[2] = 18;
	ins->reg_C0_dt2_d2r[2] = 0;
	ins->reg_E0_d1l_rr[2] = (5 << 4) | 10;

	ins->reg_40_dt1_mul[3] = 0;
	ins->reg_60_tl[3] = 6;
	ins->reg_80_ks_ar[3] = 31;
	ins->reg_A0_ame_d1r[3] = 5;
	ins->reg_C0_dt2_d2r[3] = 5;
	ins->reg_E0_d1l_rr[3] = (14 << 4) | 15;

	ins++;

	ins->reg_20_pan_fl_con = 0xC0 | (5 << 3) | 4;
	ins->reg_38_pms_ams = 0;

	ins->reg_40_dt1_mul[0] = 1;
	ins->reg_60_tl[0] = 11;
	ins->reg_80_ks_ar[0] = 30;
	ins->reg_A0_ame_d1r[0] = 8;
	ins->reg_C0_dt2_d2r[0] = 0;
	ins->reg_E0_d1l_rr[0] = (4 << 4) | 15;

	ins->reg_40_dt1_mul[1] = 1;
	ins->reg_60_tl[1] = 10;
	ins->reg_80_ks_ar[1] = 30;
	ins->reg_A0_ame_d1r[1] = 6;
	ins->reg_C0_dt2_d2r[1] = 0;
	ins->reg_E0_d1l_rr[1] = (4 << 4) | 15;

	ins->reg_40_dt1_mul[2] = 0;
	ins->reg_60_tl[2] = 22;
	ins->reg_80_ks_ar[2] = 30;
	ins->reg_A0_ame_d1r[2] = 11;
	ins->reg_C0_dt2_d2r[2] = 0;
	ins->reg_E0_d1l_rr[2] = (10 << 4) | 15;

	ins->reg_40_dt1_mul[3] = 0;
	ins->reg_60_tl[3] = 8;
	ins->reg_80_ks_ar[3] = 30;
	ins->reg_A0_ame_d1r[3] = 3;
	ins->reg_C0_dt2_d2r[3] = 0;
	ins->reg_E0_d1l_rr[3] = (0 << 4) | 15;
}

void set_demo_meta(void)
{
	xt.track.num_phrases = 16;

	for (int i = 0; i < xt.track.num_phrases; i++)
	{
		for (int c = 0; c < 8; c++)
		{
			xt.track.frames[i].row_idx[c] = i;
		}
	}

	xt.track.num_frames = 32;
	xt.track.num_instruments = 1;

	xt.track.ticks_per_row = 6;
	xt.track.timer_period = 0xABCD;

	xt.track.phrase_length = 32;
	xt.track.loop_point = 1;
}

int main(int argc, char **argv)
{
	_dos_super(0);
//	int old_crt_mode = _iocs_crtmod(7);
	_iocs_b_curoff();
	_iocs_g_clr_on();
	_iocs_b_putmes(1, 0, 0, 20, "Loading XTracker...");

	_iocs_bgtextcl(0, 0x20);
	_iocs_bgtextcl(1, 0x20);

	if (!video_init())
	{
		fprintf(stderr, "Couldn't start Xtracker.\n");
	}

	xt_init(&xt);
	x68k_wait_for_vsync();
	xt_track_renderer_init(&renderer);
	xt_keys_init(&keys);
	xt_phrase_editor_init(&phrase_editor);

	// Set up xt with some test data
	set_demo_meta();
	set_demo_instruments();

	_iocs_b_putmes(1, 0, 0, 20, "Welcome to XTracker");

	for (int i = 0; i < 15; i++)
	{
		_iocs_b_putmes(1, 0, i, 64, "                                               ");
	}

	// The main loop.
	while (!xt_keys_pressed(&keys, XT_KEY_ESC))
	{
		if (xt_keys_pressed(&keys, XT_KEY_CR))
		{
			// Take the playback position from the editor.
			if (!xt.playing)
			{
				// Hold shift to play the same phrase repeatedly.
				xt_start_playing(&xt, phrase_editor.frame, xt_keys_held(&keys, XT_KEY_SHIFT));
			}
			else
			{
				xt_stop_playing(&xt);
			}
		}

		// TODO: These should be interrupt-driven using the OPM's timer.
		xt_tick(&xt);
		xt_phrase_editor_tick(&phrase_editor, &xt.track, &keys);
		xt_update_opm_registers(&xt);

		xt_vbl_wait(0);
		xt_keys_update(&keys);

		// During playback, the scroll position and displayed frame are
		// commanded by the main Xt object.
		x68k_pcg_set_bg0_xscroll(0);
		x68k_pcg_set_bg1_xscroll(0);
		if (xt.playing)
		{
			const uint16_t yscroll = (xt.current_phrase_row - 16) * 8;
			x68k_pcg_set_bg0_yscroll(yscroll);
			x68k_pcg_set_bg1_yscroll(yscroll);
			xt_track_renderer_tick(&renderer, &xt, xt.current_frame);
		}
		else
		{
			const uint16_t cam_y = -128 + phrase_editor.row * 8;
			x68k_pcg_set_bg0_yscroll(cam_y);
			x68k_pcg_set_bg1_yscroll(cam_y);
			xt_phrase_editor_update_renderer(&phrase_editor, &renderer);
			xt_track_renderer_tick(&renderer, &xt, phrase_editor.frame);
			xt_phrase_editor_draw_cursor(&phrase_editor, 0, cam_y);
		}
		x68k_pcg_finish_sprites();
	}
	xt_vbl_shutdown();

//	_iocs_crtmod(old_crt_mode);
	_iocs_b_curon();
	_iocs_g_clr_on();

	return 0;
}
