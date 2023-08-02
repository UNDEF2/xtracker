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
#include "core/display_config.h"
#include "ui/fnlabels.h"
#include "ui/track_render.h"
#include "xt.h"
#include "xt_arrange_render.h"
#include "xt_phrase_editor.h"
#include "xt_keys.h"
#include "xt_irq.h"
#include "xt_palette.h"
#include "xt_instrument.h"

static Xt s_xt;
static XtKeys s_keys;
static XtArrangeRenderer s_arrange_renderer;
static XtTrackRenderer s_track_renderer;
static XtPhraseEditor s_phrase_editor;

//
// Test Code
//

void draw_mock_ui(void)
{
	// Draw bottom legend
	ui_fnlabel_set(0, "FILE");
	ui_fnlabel_set(1, "PATTERN");
	ui_fnlabel_set(2, "META");
	ui_fnlabel_set(3, "INSTR");
	ui_fnlabel_set(4, "ARRANGE");
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
	s_xt.track.num_phrases = 16;

	for (int i = 0; i < s_xt.track.num_phrases; i++)
	{
		for (int c = 0; c < 8; c++)
		{
			s_xt.track.frames[i].phrase_id[c] = i;
		}
	}

	s_xt.track.num_frames = 64;
	s_xt.track.num_instruments = 1;

	s_xt.track.ticks_per_row = 6;
	s_xt.track.timer_period = 0xABCD;

	s_xt.track.phrase_length = 32;
	s_xt.track.loop_point = 1;

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
	XT_UI_FOCUS_INSTRUMENT_LIST,
	XT_UI_FOCUS_INSTRUMENT_EDIT,
	XT_UI_FOCUS_INSTRUMENT_FILE,
	XT_UI_FOCUS_META,
	XT_UI_FOCUS_ADPCM_MAPPING,
	XT_UI_FOCUS_ADPCM_FILE,
	// TODO: Instrument file dialogue, ADPCM file dialogue, ADPCM mapping
} XtUiFocus;

int main(int argc, char **argv)
{
	_dos_super(0);

	display_config_init();

	cgprint_load("RES\\CGFNT8.BIN");

	xt_irq_init();
	xt_palette_init();

	xt_init(&s_xt);

	xb_mfp_set_interrupt_enable(XB_MFP_INT_VDISP, true);
	xb_mfp_set_interrupt_enable(XB_MFP_INT_FM_SOUND_SOURCE, true);

	xt_irq_wait_vbl();

	xt_track_renderer_init(&s_track_renderer);
	xt_arrange_renderer_init(&s_arrange_renderer, &s_xt.track);
	xt_keys_init(&s_keys);
	xt_phrase_editor_init(&s_phrase_editor, &s_xt.track);

	// Set up xt with some test data
	set_demo_meta();
	set_demo_instruments();

	int elapsed = 0;

	draw_mock_ui();

	XtUiFocus focus = 0;

	// The main loop.
	while (!xt_keys_pressed(&s_keys, XT_KEY_ESC))
	{
		xt_keys_poll(&s_keys);

		XtKeyEvent key_event;

		switch (focus)
		{
			default:
				break;
			case XT_UI_FOCUS_PATTERN:
				// TODO: tick the playback engine and register updates based
				// on the OPM timer.
				xt_poll(&s_xt);

				s_phrase_editor.visible_channels = s_track_renderer.visible_channels;
				while (xt_keys_event_pop(&s_keys, &key_event))
				{
					if (key_event.name == XT_KEY_HELP) display_config_cycle_modes();
					if (!s_xt.playing)
					{
						xt_phrase_editor_on_key(&s_phrase_editor, &s_xt.track, key_event);
					}
				}

				if (s_xt.playing)
				{
					if (xt_keys_pressed(&s_keys, XT_KEY_CR))
					{
						xt_stop_playing(&s_xt);
					}
					// Focus the "camera" down a little bit to make room for the HUD.
					const int16_t yscroll = (s_xt.current_phrase_row - 16) * 8;
					xt_track_renderer_set_camera(&s_track_renderer, xt_phrase_editor_get_cam_x(&s_phrase_editor), yscroll);
					xt_track_renderer_tick(&s_track_renderer, &s_xt, s_xt.current_frame);
					xt_arrange_renderer_tick(&s_arrange_renderer, &s_xt.track, s_xt.current_frame, -1);
				}
				else
				{
					if (xt_keys_pressed(&s_keys, XT_KEY_CR))
					{
						// playback position is taken from the editor.
						xt_start_playing(&s_xt, s_phrase_editor.frame,
						                 xt_keys_held(&s_keys, XT_KEY_SHIFT));
					}
					xt_track_renderer_tick(&s_track_renderer, &s_xt, s_phrase_editor.frame);
					xt_arrange_renderer_tick(&s_arrange_renderer, &s_xt.track, s_phrase_editor.frame, s_phrase_editor.column);
					xt_phrase_editor_update_renderer(&s_phrase_editor, &s_track_renderer);
				}
				xt_update_opm_registers(&s_xt);
				break;
		}

		xt_irq_wait_vbl();

		elapsed++;
	}

	xt_irq_shutdown();

	display_config_shutdown();
	_dos_kflushio(0xFF);

	return 0;
}
