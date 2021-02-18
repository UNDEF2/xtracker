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
#include "cgprint.h"
#include "irq.h"
#include "xt.h"
#include "xt_render.h"
#include "xt_phrase_editor.h"
#include "xt_keys.h"
#include "xt_irq.h"
#include "xt_display.h"

static Xt xt;
static XtTrackRenderer renderer;
static XtKeys keys;
static XtPhraseEditor phrase_editor;
static XtDisplay display;

static int16_t s_old_crt_mode;

// 512 x 256 @ 55Hz
static const XtDisplayMode mode_15k =
#define CRT_FLAGS 0x0001
{
	{
		0x004C, 0x0006, 0x0008, 0x0048,
		0x011C, 0x0002, 0x0014, 0x0114,
		0x20, CRT_FLAGS
	},
	{
		0x00FF, 0x000C, 0x0014, 0x0000
	},
	{
		(CRT_FLAGS >> 8), 0x31E4, 0x007F
	}
#undef CRT_FLAGS
};

// 512 x 512 @ 55Hz (line doubled)
static const XtDisplayMode mode_31k =
#define CRT_FLAGS 0x0011
{
	{
		0x005B, 0x0008, 0x0011, 0x0051,
		0x0237, 0x0005, 0x0026, 0x0226,
		0x1B, CRT_FLAGS
	},
	{
		0x00FF, 0x0015, 0x0026, 0x0010
	},
	{
		(CRT_FLAGS >> 8), 0x31E4, 0x007F
	}
#undef CRT_FLAGS
};

static const XtDisplayMode *display_modes[] =
{
	&mode_15k,
	&mode_31k,
};

//static uint8_t txfnt[2048];
//static uint8_t cgfnt[8192];
static uint8_t cgfnt8[16384];

static void draw_fnc_label(int16_t i, const char *s)
{
	static const int16_t box_w = 1 + 6 * 7;
	static const int16_t box_h = 9;
	static const int16_t box_x_margin = 6;
	static const int16_t box_y = 256 - box_h - 1;

	int16_t box_x = box_x_margin + (i * (box_w + box_x_margin)) + ((i >= 5) ? (2 * box_x_margin) : 0);
	cgbox(0, 15, box_x, box_y, box_x + box_w, box_y + box_h);
	cgprint(cgfnt8, 0, 1, s, box_x + 1, box_y + 1);
}

// Give it a try using just the 
void draw_mock_ui(void)
{
	FILE *f = fopen("CGFNT8.BIN", "rb");
	if (!f)
	{
		fprintf(stderr, "Couldn't read font\n");
	}
	fread(cgfnt8, 1, ARRAYSIZE(cgfnt8), f);
	fclose(f);
	for (int i = 0; i < 24; i++)
	{
		cgprint(cgfnt8, 0, i, "colors", i, 7 * i);
	}
	// Draw bottom legend
	draw_fnc_label(0, "FILE");
	draw_fnc_label(1, "PATTERN");
	draw_fnc_label(2, "META");
	draw_fnc_label(3, "INSTR");
	draw_fnc_label(4, "ARRANGE");
	draw_fnc_label(5, "");
	draw_fnc_label(6, "");
	draw_fnc_label(7, "");
	draw_fnc_label(8, "");
	draw_fnc_label(9, "");
}

int video_init(void)
{
	xt_display_init(&display, display_modes, ARRAYSIZE(display_modes));
	xt_irq_init();

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

	x68k_pcg_set_bg0_txsel(0);
	x68k_pcg_set_bg1_txsel(1);
	x68k_pcg_set_bg0_enable(1);
	x68k_pcg_set_bg1_enable(1);

	x68k_pcg_set_bg0_xscroll(0);
	x68k_pcg_set_bg1_xscroll(0);

	x68k_pcg_set_bg0_yscroll(0);
	x68k_pcg_set_bg1_yscroll(0);

	x68k_vbl_wait_for_vblank();

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

	xt.track.num_frames = 64;
	xt.track.num_instruments = 1;

	xt.track.ticks_per_row = 6;
	xt.track.timer_period = 0xABCD;

	xt.track.phrase_length = 32;
	xt.track.loop_point = 1;
}

static int main_init(void)
{
	// Disable some Human68k stuff we won't be using
	_iocs_g_clr_on();
	_iocs_b_curoff();
	_iocs_ms_curof();
	_iocs_skey_mod(0, 0, 0);

	// Store the old video mode so we can restore it on exit.
	s_old_crt_mode = _iocs_crtmod(-1);

	// Let Human68k know that we're using all planes
	// (this prevents it from popping up the calculator or softkey).
	_iocs_tgusemd(0, 2);  // Grahpics planes, in use
	_iocs_tgusemd(1, 0);  // Let Human use the text planes.

	if (!video_init())
	{
		fprintf(stderr, "Couldn't start Xtracker.\n");
		return 0;
	}

	_iocs_b_clr_al();

	return 1;
}

static void main_shutdown(void)
{
	_iocs_g_clr_on();
	_iocs_skey_mod(-1, 0, 0);
	_iocs_b_curon();
	_iocs_crtmod(s_old_crt_mode);

	_iocs_tgusemd(0, 0);
	_iocs_tgusemd(1, 0);

	_dos_kflushio(0xFF);
}

int main(int argc, char **argv)
{
	_dos_super(0);

	if (!main_init())
	{
		return -1;
	}

	xt_init(&xt);
	x68k_wait_for_vsync();
	xt_track_renderer_init(&renderer);
	xt_keys_init(&keys);
	xt_phrase_editor_init(&phrase_editor);

	// Set up xt with some test data
	set_demo_meta();
	set_demo_instruments();

	int elapsed = 0;

	draw_mock_ui();

	// The main loop.
	while (!xt_keys_pressed(&keys, XT_KEY_ESC))
	{
		xt_keys_poll(&keys);

		XtKeyEvent key_event;

		switch (xt.focus)
		{
			default:
				break;
			case XT_UI_FOCUS_PATTERN:
				// TODO: tick the playback engine and register updates based
				// on the OPM timer.
				xt_poll(&xt);

				// TODO: This is a hack, if this works it needs to be done properly
				const XtDisplayMode *mode = xt_display_get_mode(&display);
				const int visible_channels = (mode->crtc.hdisp_end - mode->crtc.hdisp_start) / 8;
				renderer.visible_channels = visible_channels;
				phrase_editor.visible_channels = visible_channels;
				while (xt_keys_event_pop(&keys, &key_event))
				{
					xt_display_on_key(&display, key_event);
					if (!xt.playing)
					{
						xt_phrase_editor_on_key(&phrase_editor, &xt.track, key_event);
					}
				}
				if (xt.playing)
				{
					if (xt_keys_pressed(&keys, XT_KEY_CR))
					{
						xt_stop_playing(&xt);
						xt.playing = 0;
					}
					const int16_t yscroll = (xt.current_phrase_row - 16) * 8;
					xt_track_renderer_set_camera(&renderer, xt_phrase_editor_get_cam_x(&phrase_editor), yscroll);
					xt_track_renderer_tick(&renderer, &xt, xt.current_frame);
				}
				else
				{
					if (xt_keys_pressed(&keys, XT_KEY_CR))
					{
						// Playback position is taken from the editor.
						xt_start_playing(&xt, phrase_editor.frame,
						                 xt_keys_held(&keys, XT_KEY_SHIFT));
						xt.playing = 1;
					}
					xt_phrase_editor_update_renderer(&phrase_editor, &renderer);
					xt_track_renderer_tick(&renderer, &xt, phrase_editor.frame);
				}
				xt_update_opm_registers(&xt);
				break;
		}

		x68k_pcg_finish_sprites();
		xt_irq_wait_vbl();
		elapsed++;
	}
	xt_irq_shutdown();

	main_shutdown();

	return 0;
}
