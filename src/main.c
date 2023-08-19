#include <stdio.h>
#include <dos.h>
#include <iocs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "xbase/crtc.h"
#include "xbase/memmap.h"
#include "xbase/vidcon.h"
#include "xbase/mfp.h"

#include "common.h"
#include "cgprint.h"
#include "phrase_editor.h"
#include "arrange_editor.h"
#include "core/display_config.h"
#include "util/txprint.h"
#include "ui/arrange_render.h"
#include "ui/cursor.h"
#include "ui/fnlabels.h"
#include "ui/instrument_render.h"
#include "ui/track_render.h"

#include "xt.h"
#include "xt_keys.h"
#include "xt_irq.h"
#include "xt_palette.h"
#include "xt_instrument.h"

static Xt s_xt;
static XtKeys s_keys;
static XtArrangeRenderer s_arrange_renderer;
static XtInstrumentRenderer s_instrument_renderer;
static XtTrackRenderer s_track_renderer;
static XtPhraseEditor s_phrase_editor;
static XtArrangeEditor s_arrange_editor;

//
// Test Code
//

void draw_mock_ui(void)
{
	txprintf(64-9, 0, 1, "XTracker ");
}

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
	XT_UI_FOCUS_PATTERN_EDIT,
	XT_UI_FOCUS_ARRANGE_EDIT,
	XT_UI_FOCUS_INSTRUMENT_LIST,
	XT_UI_FOCUS_INSTRUMENT_EDIT,
	XT_UI_FOCUS_INSTRUMENT_FILE,
	XT_UI_FOCUS_META_EDIT,
	XT_UI_FOCUS_ADPCM_MAPPING,
	XT_UI_FOCUS_ADPCM_FILE,
	XT_UI_TRACK_FILE,
} XtUiFocus;

void maybe_set_fnlabels(XtUiFocus next_focus, XtUiFocus focus)
{
	if (next_focus == focus) return;
	switch (next_focus)
	{
		case XT_UI_FOCUS_PATTERN_EDIT:
			xt_phrase_editor_set_fnlabels();
			break;
		case XT_UI_FOCUS_ARRANGE_EDIT:
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

static void toggle_playback(XtKeyEvent key_event)
{
	if (s_xt.playing)
	{
		if (key_event.name == XT_KEY_CR)
		{
			xt_stop_playing(&s_xt);
		}
	}
	else
	{
		if (key_event.name == XT_KEY_CR)
		{
			xt_start_playing(&s_xt, s_phrase_editor.frame,
			                 xt_keys_held(&s_keys, XT_KEY_SHIFT));
		}
		xt_phrase_editor_on_key(&s_phrase_editor, &s_xt.track, key_event);
	}
}

int main(int argc, char **argv)
{
	_dos_super(0);

	display_config_init();
	txprint_init();

	cgprint_load("RES\\CGFNT8.BIN");

	xt_init(&s_xt);

	xt_irq_init();
	xt_palette_init();
	xt_keys_init(&s_keys);
	xt_cursor_init();

	xt_track_renderer_init(&s_track_renderer);
	xt_arrange_renderer_init(&s_arrange_renderer);
	xt_arrange_editor_init(&s_arrange_editor, &s_arrange_renderer);
	xt_phrase_editor_init(&s_phrase_editor, &s_xt.track);
	xt_instrument_renderer_init(&s_instrument_renderer, &s_xt.track, &s_phrase_editor);

	xb_mfp_set_interrupt_enable(XB_MFP_INT_VDISP, true);
	xb_mfp_set_interrupt_enable(XB_MFP_INT_FM_SOUND_SOURCE, true);

	//s_phrase_editor.visible_channels = s_track_renderer.visible_channels;

	// Set up xt with some test data
	set_demo_meta();
	set_demo_instruments();

	draw_mock_ui();

	XtUiFocus focus = -1;
	XtUiFocus next_focus = XT_UI_FOCUS_PATTERN_EDIT;

	uint32_t elapsed = 0;
	bool quit = false;

	// The main loop.
	while (!quit)
	{
		//
		// Input handling.
		//

		xt_keys_poll(&s_keys);

		XtKeyEvent key_event;

		maybe_set_fnlabels(next_focus, focus);
		focus = next_focus;

		while (xt_keys_event_pop(&s_keys, &key_event))
		{
			// General key inputs that are always active.
			switch (key_event.name)
			{
				default:
					break;
				case XT_KEY_CR:
					switch (focus)
					{
						case XT_UI_FOCUS_PATTERN_EDIT:
							toggle_playback(key_event);

						default:
							break;
					}

					break;

				case XT_KEY_HELP:
					display_config_cycle_modes();
					break;

				case XT_KEY_BREAK:
					quit = true;
					break;

				// Focus changes
				case XT_KEY_F7:
					next_focus = XT_UI_FOCUS_PATTERN_EDIT;
					break;

				case XT_KEY_F8:
					next_focus = XT_UI_FOCUS_INSTRUMENT_LIST;
					break;

				case XT_KEY_F9:
					next_focus = XT_UI_FOCUS_ARRANGE_EDIT;
					break;

				case XT_KEY_F10:
					next_focus = XT_UI_FOCUS_META_EDIT;
					break;
			}

			// Forwarding of key events to focus areas.
			switch (focus)
			{
				default:
					break;

				case XT_UI_FOCUS_PATTERN_EDIT:
					if (!s_xt.playing)
					{
						xt_phrase_editor_on_key(&s_phrase_editor,
					                            &s_xt.track, key_event);
						s_arrange_editor.frame = s_phrase_editor.frame;
						s_arrange_editor.column = s_phrase_editor.column;
					}
					break;

				case XT_UI_FOCUS_ARRANGE_EDIT:
					if (!s_xt.playing)
					{
						xt_arrange_editor_on_key(&s_arrange_editor,
					                             &s_xt.track, key_event);
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

		if (s_xt.playing)
		{
			// Focus the "camera" down a little bit to make room for the HUD.
			const int16_t yscroll = (s_xt.current_phrase_row - 16) * 8;
			xt_track_renderer_set_camera(&s_track_renderer,
			                             xt_phrase_editor_get_cam_x(&s_phrase_editor),
			                             yscroll);
		}
		else
		{
			xt_phrase_editor_update_renderer(&s_phrase_editor, &s_track_renderer);
		}

		const int16_t scroll_frame = (s_xt.playing ? s_xt.current_frame : s_phrase_editor.frame);
		xt_track_renderer_tick(&s_track_renderer, &s_xt, scroll_frame);
		xt_arrange_renderer_tick(&s_arrange_renderer, &s_xt.track, s_arrange_editor.frame, s_arrange_editor.column);
		xt_instrument_renderer_tick(&s_instrument_renderer, &s_xt.track, &s_phrase_editor);

		elapsed++;
	}

	xt_irq_shutdown();
	display_config_shutdown();

	_dos_kflushio(0xFF);

	return 0;
}
