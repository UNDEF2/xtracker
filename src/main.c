#include <stdio.h>
#include <dos.h>
#include <iocs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "xbase/crtc.h"
#include "xbase/keys.h"
#include "xbase/memmap.h"
#include "xbase/vidcon.h"
#include "xbase/mfp.h"

#include "common.h"
#include "phrase_editor.h"
#include "arrange_editor.h"
#include "core/display_config.h"
#include "util/cgprint.h"
#include "util/txprint.h"
#include "ui/arrange_render.h"
#include "ui/backing.h"
#include "ui/cursor.h"
#include "ui/chanlabels.h"
#include "ui/fnlabels.h"
#include "ui/instrument_render.h"
#include "ui/track_render.h"

#include "xt.h"
#include "xt_irq.h"
#include "xt_palette.h"
#include "xt_instrument.h"

static Xt s_xt;
static XtArrangeRenderer s_arrange_renderer;
static XtInstrumentRenderer s_instrument_renderer;
static XtTrackRenderer s_track_renderer;
static XtPhraseEditor s_phrase_editor;
static XtArrangeEditor s_arrange_editor;

//
// Test Code
//

void set_demo_instruments(void)
{
	XtInstrument *ins = &s_xt.track.instruments[0];

	// The bass from Private Eye (Daiginjou)
	memset(ins, 0, sizeof(*ins));
	ins->type = XT_CHANNEL_OPM;
	ins->valid = true;
	ins->opm.fl = 7;
	ins->opm.con = 3;

	ins->opm.mul[0] = 8;
	ins->opm.tl[0] = 30;
	ins->opm.ar[0] = 27;
	ins->opm.d1r[0] = 14;
	ins->opm.d1l[0] = 3;
	ins->opm.rr[0] = 10;

	ins->opm.mul[1] = 2;
	ins->opm.tl[1] = 45;
	ins->opm.ar[1] = 31;
	ins->opm.d1r[1] = 12;
	ins->opm.d1l[1] = 3;
	ins->opm.rr[1] = 10;

	ins->opm.mul[2] = 0;
	ins->opm.tl[2] = 15;
	ins->opm.ar[2] = 31;
	ins->opm.d1r[2] = 18;
	ins->opm.d1l[2] = 5;
	ins->opm.rr[2] = 10;

	ins->opm.mul[3] = 0;
	ins->opm.tl[3] = 6;
	ins->opm.ar[3] = 31;
	ins->opm.d1r[3] = 5;
	ins->opm.d2r[3] = 5;
	ins->opm.d1l[3] = 14;
	ins->opm.rr[3] = 15;

	ins++;
	memset(ins, 0x21, sizeof(*ins));
}

void set_demo_meta(void)
{
	s_xt.track.num_frames = 1;
	s_xt.track.num_instruments = 1;

	s_xt.track.ticks_per_row = 6;
	s_xt.track.timer_period = 0xABCD;

	s_xt.track.phrase_length = 32;
	s_xt.track.loop_point = 0;

	for (int16_t i = 0; i < ARRAYSIZE(s_xt.track.channel_data); i++)
	{
		XtTrackChannelData *data = &s_xt.track.channel_data[i];
		if (i < 8)
		{
			data->type = XT_CHANNEL_OPM;
			data->voice_number = i;
		}
		else
		{
			data->type = XT_CHANNEL_ADPCM;
			data->voice_number = i;
		}
	}
}

//
// Main
//

// TODO: xt_ui.c
typedef enum XtUiFocus
{
	XT_UI_FOCUS_PATTERN,
	XT_UI_FOCUS_ARRANGE,
	XT_UI_FOCUS_INSTRUMENT,
	XT_UI_FOCUS_META,
	XT_UI_FOCUS_ADPCM_MAPPING,
	XT_UI_FOCUS_ADPCM_FILE,
	XT_UI_FOCUS_FILE_DIALOGUE,
} XtUiFocus;

static void maybe_set_chanlabels(int16_t cam_x)
{
	static int16_t s_last_cam_x = -1;
	if (s_last_cam_x != cam_x)
	{
		const int16_t left_visible_channel = cam_x / (XT_RENDER_CELL_W_PIXELS * XT_RENDER_CELL_CHARS);
		for (int16_t i = 0; i < XT_RENDER_VISIBLE_CHANNELS; i++)
		{
			ui_chanlabel_set(i, left_visible_channel + i);
		}
		s_last_cam_x = cam_x;
	}
}

static void maybe_set_fnlabels(XBKeyEvent *ev, XtUiFocus next_focus, XtUiFocus focus)
{
	bool want_redraw = next_focus != focus;
	if (ev)
	{
		if (ev->modifiers & XB_KEY_MOD_IS_REPEAT) return;
		if (ev->name == XB_KEY_CTRL) want_redraw = true;
	}
	if (!want_redraw) return;
	switch (next_focus)
	{
		case XT_UI_FOCUS_PATTERN:
			xt_phrase_editor_set_fnlabels(xb_key_on(XB_KEY_CTRL));
			break;
		case XT_UI_FOCUS_ARRANGE:
			xt_arrange_editor_set_fnlabels();
			break;
		default:
			break;
	}

	ui_fnlabel_set(5, "File");
	ui_fnlabel_set(6, "Pattern");
	ui_fnlabel_set(7, "Instr");
	ui_fnlabel_set(8, "Arrange");
	ui_fnlabel_set(9, "Meta");

}

static void toggle_playback(XBKeyEvent key_event)
{
	if (key_event.modifiers & XB_KEY_MOD_KEY_UP) return;
	if (s_xt.playing)
	{
		if (key_event.name == XB_KEY_CR)
		{
			xt_stop_playing(&s_xt);
		}
	}
	else
	{
		if (key_event.name == XB_KEY_CR)
		{
			xt_start_playing(&s_xt, s_phrase_editor.frame,
			                 xb_key_on(XB_KEY_SHIFT));
		}
		xt_phrase_editor_on_key(&s_phrase_editor, &s_xt.track, key_event);
	}
}

static void hack_load_track(const char *fname)
{
	FILE *f = fopen(fname, "rb");
	if (!f) return;

	// fread(&s_xt.track, 1, sizeof(s_xt.track), f);

	fclose(f);
}

static void hack_save_track(const char *fname)
{
	FILE *f = fopen(fname, "wb");
	if (!f) return;

	// fwrite(&s_xt.track, 1, sizeof(s_xt.track), f);

	fclose(f);
}

static void update_scroll(void)
{
	// Set camera / scroll for pattern field.
	// Focus the "camera" down a little bit to make room for the HUD.
	if (xt_is_playing(&s_xt))
	{
		const int16_t yscroll = (s_xt.current_phrase_row - 16) * 8;
		xt_track_renderer_set_camera(&s_track_renderer,
		                             xt_phrase_editor_get_cam_x(&s_phrase_editor),
		                             yscroll);
	}
	else
	{
		// TODO: Split out side effects of repainting or give a better name.
		xt_track_renderer_set_camera(&s_track_renderer,
		                             xt_phrase_editor_get_cam_x(&s_phrase_editor),
		                             xt_phrase_editor_get_cam_y(&s_phrase_editor));
	}
}

int main(int argc, char **argv)
{
	_dos_super(0);

	display_config_init();
	txprint_init();

	cgprint_load("RES\\CGDAT.BIN");
	txprintf(0, 16, 1, "");

	xt_init(&s_xt);

	xt_irq_init();
	xt_palette_init();

	xb_keys_init(NULL);
	xt_cursor_init();

	xt_track_renderer_init(&s_track_renderer);
	xt_arrange_renderer_init(&s_arrange_renderer);
	xt_arrange_editor_init(&s_arrange_editor, &s_arrange_renderer);
	xt_phrase_editor_init(&s_phrase_editor, &s_xt.track);
	xt_instrument_renderer_init(&s_instrument_renderer, &s_xt.track, &s_phrase_editor);

	xb_mfp_set_interrupt_enable(XB_MFP_INT_VDISP, true);
	xb_mfp_set_interrupt_enable(XB_MFP_INT_FM_SOUND_SOURCE, true);

	ui_backing_draw();

	//s_phrase_editor.visible_channels = s_track_renderer.visible_channels;

	// Set up xt with some test data
	set_demo_meta();
	set_demo_instruments();

	// HACK LOADING
	const char *filename = NULL;

	if (argc > 1)
	{
		filename = argv[1];
	}

	if (filename) hack_load_track(filename);

	XtUiFocus focus = -1;
	XtUiFocus next_focus = XT_UI_FOCUS_PATTERN;

	uint32_t elapsed = 0;
	bool quit = false;

	// The main loop.
	while (!quit)
	{
		//
		// Input handling.
		//

		xb_keys_poll();

		XBKeyEvent key_event;

		maybe_set_fnlabels(NULL, next_focus, focus);
		focus = next_focus;

		while (xb_keys_event_pop(&key_event))
		{
			maybe_set_fnlabels(&key_event, next_focus, focus);

			if (key_event.modifiers & XB_KEY_MOD_KEY_UP) continue;
			// General key inputs that are always active.
			switch (key_event.name)
			{
				default:
					break;
				case XB_KEY_CR:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					toggle_playback(key_event);
					break;

				case XB_KEY_HELP:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					display_config_cycle_modes();
					break;

				case XB_KEY_BREAK:
					quit = true;
					break;

				// Save
				case XB_KEY_F6:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					if (filename) hack_save_track(filename);
					break;

				// Focus changes
				case XB_KEY_F7:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					next_focus = XT_UI_FOCUS_PATTERN;
					break;

				case XB_KEY_F8:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					next_focus = XT_UI_FOCUS_INSTRUMENT;
					break;

				case XB_KEY_F9:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					next_focus = XT_UI_FOCUS_ARRANGE;
					break;

				case XB_KEY_F10:
					if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
					next_focus = XT_UI_FOCUS_META;
					break;
			}

			// Forwarding of key events to focus areas.
			switch (focus)
			{
				default:
					break;

				case XT_UI_FOCUS_PATTERN:
					if (!s_xt.playing)
					{
						xt_phrase_editor_on_key(&s_phrase_editor,
						                        &s_xt.track, key_event);
						s_arrange_editor.frame = s_phrase_editor.frame;
						s_arrange_editor.column = s_phrase_editor.column;
					}
					break;

				case XT_UI_FOCUS_ARRANGE:
					if (!s_xt.playing)
					{
						xt_arrange_editor_on_key(&s_arrange_editor,
						                         &s_xt.track,
						                         &s_track_renderer, key_event);
						s_phrase_editor.frame = s_arrange_editor.frame;
						s_phrase_editor.column = s_arrange_editor.column;
					}
					break;
			}
		}

		//
		// Main engine poll.
		//

		// TODO: tick based on timer
		xt_poll(&s_xt);
		xt_update_opm_registers(&s_xt);

		// Rendering is done during VBlank, ideally.
		xt_irq_wait_vbl();
		xt_cursor_update();

		//
		// Rendering.
		//

		switch (focus)
		{
			default:
				break;
			case XT_UI_FOCUS_PATTERN:
				xt_phrase_editor_update_renderer(&s_phrase_editor, &s_track_renderer);
				xt_arrange_renderer_tick(&s_arrange_renderer, &s_xt.track,
				                         s_arrange_editor.frame, s_arrange_editor.column);
				break;
			case XT_UI_FOCUS_ARRANGE:
				xt_arrange_renderer_tick(&s_arrange_renderer, &s_xt.track,
				                         s_arrange_editor.frame, s_arrange_editor.column);
				break;
			case XT_UI_FOCUS_INSTRUMENT:
				xt_instrument_renderer_tick(&s_instrument_renderer, &s_xt.track,
				                            &s_phrase_editor, s_phrase_editor.instrument);
				xt_arrange_renderer_request_redraw(&s_arrange_renderer);
				break;
		}

		update_scroll();

		// Update the track renderer to repaint pattern data.
		const int16_t displayed_frame = (s_xt.playing ? s_xt.current_frame : s_phrase_editor.row);
		xt_track_renderer_tick(&s_track_renderer, &s_xt, displayed_frame);

		maybe_set_chanlabels(xt_phrase_editor_get_cam_x(&s_phrase_editor));
		elapsed++;
	}

	xt_irq_shutdown();
	display_config_shutdown();

	_dos_kflushio(0xFF);

	return 0;
}
