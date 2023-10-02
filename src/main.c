#include <stdio.h>
#include <dos.h>
#include <iocs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "xbase/crtc.h"
#include "xbase/ipl.h"
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
#include "ui/metrics.h"
#include "ui/regdata_render.h"
#include "ui/track_render.h"

#include "xt.h"
#include "xt_palette.h"
#include "xt_instrument.h"

static Xt s_xt;
static XtTrack s_track;
static XtArrangeRenderer s_arrange_renderer;
static XtRegdataRenderer s_regdata_renderer;
static XtTrackRenderer s_track_renderer;
static XtPhraseEditor s_phrase_editor;
static XtArrangeEditor s_arrange_editor;

//
// Interrupts (OPM timer and Vertical raster)
//

static void (*s_old_vbl_isr)(void);
static uint8_t s_old_ipl;
static volatile bool s_vbl_hit;

static void XB_ISR s_isr_opm(void)
{
	xt_poll(&s_xt);
	xt_update_opm_registers(&s_xt);
	xb_opm_set_timer_flags(OPM_TIMER_FLAG_IRQ_EN_A | OPM_TIMER_FLAG_LOAD_A | OPM_TIMER_FLAG_F_RESET_A);
}

static void XB_ISR s_isr_vbl(void)
{
	s_vbl_hit = true;
}


static void wait_for_vbl(void)
{
	s_vbl_hit = false;
	while (s_vbl_hit == false) {}
}

static bool interrupts_init(void)
{
	s_old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);

	s_old_vbl_isr = _iocs_b_intvcs(XB_MFP_INT_VDISP, s_isr_vbl);
	if (_iocs_opmintst(s_isr_opm))
	{
		xb_set_ipl(s_old_ipl);
		return false;
	}

	xb_opm_set_clka_period(0x100);  // Slow default
	xb_opm_set_timer_flags(OPM_TIMER_FLAG_IRQ_EN_A | OPM_TIMER_FLAG_LOAD_A | OPM_TIMER_FLAG_F_RESET_A);

	xb_mfp_set_interrupt_enable(XB_MFP_INT_FM_SOUND_SOURCE, true);
	xb_mfp_set_interrupt_enable(XB_MFP_INT_VDISP, true);

	xb_set_ipl(XB_IPL_ALLOW_ALL);
	return true;
}

static void interrupts_shutdown(void)
{
	_iocs_b_intvcs(XB_MFP_INT_VDISP, s_old_vbl_isr);
	_iocs_opmintst(NULL);
	_iocs_opmset(0x15, 0x30);
	xb_set_ipl(s_old_ipl);
}

//
// Test Code
//

void set_demo_instruments(void)
{
	XtInstrument *ins = &s_track.instruments[0];

	// Instrument $00 - The bass from Private Eye (Daiginjou), sort of
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

	// instrument $01 - filled crap data
	ins++;
	uint8_t val = 0;
	ins->opm.fl = val++;
	ins->opm.con = val++;
	ins->opm.pms = val++;
	ins->opm.ams = val++;

	for (uint16_t i = 0; i < XB_OPM_OP_COUNT; i++)
	{
		ins->opm.ar[i] = val++;
		ins->opm.d1r[i] = val++;
		ins->opm.d2r[i] = val++;
		ins->opm.rr[i] = val++;
		ins->opm.d1l[i] = val++;
		ins->opm.tl[i] = val++;
		ins->opm.ks[i] = val++;
		ins->opm.mul[i] = val++;
		ins->opm.dt1[i] = val++;
		ins->opm.dt2[i] = val++;
		ins->opm.ame[i] = val++;
	}
}

void set_demo_meta(void)
{
	s_track.num_frames = 1;
	s_track.num_instruments = 1;
	s_track.row_highlight[0] = 4;
	s_track.row_highlight[1] = 16;

	s_track.ticks_per_row = 6;
	s_track.timer_period = 0x30;

	s_track.phrase_length = 32;
	s_track.loop_point = 0;

	for (int16_t i = 0; i < ARRAYSIZE(s_track.channel_data); i++)
	{
		XtTrackChannelData *data = &s_track.channel_data[i];
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
	  XT_UI_FOCUS_ARRANGE_EDIT,
	  XT_UI_FOCUS_INSTRUMENT_SEL,
	    XT_UI_FOCUS_INSTRUMENT_EDIT,
	  XT_UI_FOCUS_META,
	XT_UI_FOCUS_QUIT,
} XtUiFocus;

static void draw_chanlabels(int16_t cam_x)
{
	const int16_t left_visible_channel = cam_x / (XT_RENDER_CELL_W_PIXELS * XT_RENDER_CELL_CHARS);
	for (int16_t i = 0; i < XT_RENDER_VISIBLE_CHANNELS; i++)
	{
		const XtChannelType type = s_track.channel_data[left_visible_channel + i].type;
		ui_chanlabel_set(i, left_visible_channel + i, type);
	}
}

static void maybe_set_chanlabels(int16_t cam_x)
{
	static int16_t s_last_cam_x = -1;
	if (s_last_cam_x != cam_x)
	{
		draw_chanlabels(cam_x);
		s_last_cam_x = cam_x;
	}
}

static void draw_fnlabels(XtUiFocus focus, bool ctrl_held)
{
	switch (focus)
	{
		case XT_UI_FOCUS_PATTERN:
			xt_phrase_editor_set_fnlabels(ctrl_held);
			break;
		case XT_UI_FOCUS_ARRANGE_EDIT:
			xt_arrange_editor_set_fnlabels();
			break;
		default:
			break;
	}

	ui_fnlabel_set(5, "Arrange");
	ui_fnlabel_set(6, "Instr");
	ui_fnlabel_set(7, "Meta");
	ui_fnlabel_set(8, "Pattern");
	ui_fnlabel_set(9, "File");
}

static void maybe_set_fnlabels(XBKeyEvent *ev, XtUiFocus focus)
{
	static XtUiFocus last_focus = -1;
	bool want_redraw = last_focus != focus;
	if (ev)
	{
		if (ev->modifiers & XB_KEY_MOD_IS_REPEAT) return;
		if (ev->name == XB_KEY_CTRL) want_redraw = true;
	}
	if (!want_redraw) return;
	draw_fnlabels(focus, xb_key_on(XB_KEY_CTRL));

}

static void toggle_playback(XBKeyEvent key_event)
{
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	if (key_event.modifiers & XB_KEY_MOD_KEY_UP) goto done;
	if (xt_is_playing(&s_xt))
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
		xt_phrase_editor_on_key(&s_phrase_editor, &s_track, key_event);
	}
done:
	xb_set_ipl(old_ipl);
}

static void hack_load_track(const char *fname)
{
	FILE *f = fopen(fname, "rb");
	if (!f) return;

	// fread(&s_track, 1, sizeof(s_track), f);

	fclose(f);
}

/*static void hack_save_track(const char *fname)
{
	FILE *f = fopen(fname, "wb");
	if (!f) return;

	// fwrite(&s_track, 1, sizeof(s_track), f);

	fclose(f);
}*/

static void update_scroll(void)
{
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
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
	xb_set_ipl(old_ipl);
}

static void maybe_draw_focus_label(XtUiFocus focus)
{
	static XtUiFocus s_last_focus = -1;
	if (focus == s_last_focus) return;
	s_last_focus = focus;

	const char *str = "";
	switch (focus)
	{
		default:
			break;
		case XT_UI_FOCUS_PATTERN:
			str = "Pattern Edit";
			break;
		case XT_UI_FOCUS_ARRANGE_EDIT:
			str = "Arrange Edit";
			break;
		case XT_UI_FOCUS_INSTRUMENT_SEL:
			str = "Instrument List";
			break;
		case XT_UI_FOCUS_INSTRUMENT_EDIT:
			str = "Instrument Edit";
			break;
		case XT_UI_FOCUS_META:
			str = "Meta Edit";
			break;
	}
	cgbox(XT_UI_PLANE, XT_PAL_BACK, XT_UI_FOCUS_LABEL_X, XT_UI_FOCUS_LABEL_Y,
	      XT_UI_FOCUS_LABEL_W, XT_UI_FOCUS_LABEL_H);
	cgprint(XT_UI_PLANE, XT_PAL_MAIN, str, XT_UI_FOCUS_LABEL_X, XT_UI_FOCUS_LABEL_Y);
}

static void editor_render(XtUiFocus focus)
{
	switch (focus)
	{
		default:
			break;
		case XT_UI_FOCUS_PATTERN:
			xt_phrase_editor_update_renderer(&s_phrase_editor, &s_track_renderer);
			xt_arrange_renderer_tick(&s_arrange_renderer, &s_track,
			                         s_arrange_editor.frame, s_arrange_editor.column);
			break;
		case XT_UI_FOCUS_ARRANGE_EDIT:
			xt_arrange_renderer_tick(&s_arrange_renderer, &s_track,
			                         s_arrange_editor.frame, s_arrange_editor.column);
			break;
		case XT_UI_FOCUS_INSTRUMENT_SEL:
		case XT_UI_FOCUS_INSTRUMENT_EDIT:
			xt_regdata_renderer_tick(&s_regdata_renderer, &s_track.instruments[s_phrase_editor.instrument]);
			break;
	}
	xt_cursor_update();
	update_scroll();

	// Update the track renderer to repaint pattern data.
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	const int16_t displayed_frame = (xt_is_playing(&s_xt) ? s_xt.current_frame : s_phrase_editor.frame);
	xb_set_ipl(old_ipl);
	xt_track_renderer_tick(&s_track_renderer, &s_track, displayed_frame);
	maybe_set_chanlabels(xt_phrase_editor_get_cam_x(&s_phrase_editor));
	maybe_draw_focus_label(focus);
}

static void focus_on_pattern(void)
{
	xt_phrase_editor_on_focus_acquired(&s_phrase_editor);
	xt_arrange_renderer_request_redraw(&s_arrange_renderer, /*content_only=*/false);
}

// The core of the editor is run from here, as a result of input events.
static XtUiFocus editor_logic(XtUiFocus focus)
{
	XBKeyEvent key_event;

	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	const bool playing = xt_is_playing(&s_xt);
	xb_set_ipl(old_ipl);

	while (xb_keys_event_pop(&key_event))
	{
		maybe_set_fnlabels(&key_event, focus);

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
				focus = XT_UI_FOCUS_QUIT;
				break;

			// Save

			// Focus changes
			case XB_KEY_F6:
				if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
				focus = XT_UI_FOCUS_ARRANGE_EDIT;
				xt_arrange_renderer_request_redraw(&s_arrange_renderer, /*content_only=*/false);
				break;

			case XB_KEY_F7:
				if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
				focus = XT_UI_FOCUS_INSTRUMENT_SEL;
				xt_regdata_renderer_request_redraw(&s_regdata_renderer, /*content_only=*/false);
				break;

			case XB_KEY_F8:
				if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
				focus = XT_UI_FOCUS_META;
				break;

			// TODO: Remove this case as pattern is the root, which we can get
			//       to by mashing escape.
			case XB_KEY_F9:
				if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
				focus = XT_UI_FOCUS_PATTERN;
				focus_on_pattern();
				break;

			case XB_KEY_F10:
				if (key_event.modifiers & XB_KEY_MOD_IS_REPEAT) break;
				// if (filename) hack_save_track(filename);
				break;
		}

		// Focus-specific key events.
		switch (focus)
		{
			default:
				break;

			case XT_UI_FOCUS_PATTERN:
				if (!playing)
				{
					xt_phrase_editor_on_key(&s_phrase_editor,
					                        &s_track, key_event);
					s_arrange_editor.frame = s_phrase_editor.frame;
					s_arrange_editor.column = s_phrase_editor.column;
				}
				break;

			case XT_UI_FOCUS_ARRANGE_EDIT:
				if (key_event.name == XB_KEY_ESC) focus = XT_UI_FOCUS_PATTERN;
				else if (!playing)
				{
					xt_arrange_editor_on_key(&s_arrange_editor,
					                         &s_track,
					                         &s_track_renderer, key_event);
					s_phrase_editor.frame = s_arrange_editor.frame;
					s_phrase_editor.column = s_arrange_editor.column;
				}
				break;
		}
	}
	return focus;
}

int main(int argc, char **argv)
{
	_dos_super(0);

	display_config_init();
	txprint_init();

	cgprint_load("RES\\CGDAT.BIN");
	txprintf(0, 16, 1, "");

	xt_init(&s_xt, &s_track);

	xt_palette_init();

	xb_keys_init(NULL);
	xt_cursor_init();

	xt_track_renderer_init(&s_track_renderer);
	xt_arrange_renderer_init(&s_arrange_renderer);
	xt_arrange_editor_init(&s_arrange_editor, &s_arrange_renderer);
	xt_phrase_editor_init(&s_phrase_editor, &s_track);
	xt_regdata_renderer_init(&s_regdata_renderer);

	ui_backing_draw();

	// Set up xt with some test data
	set_demo_meta();
	set_demo_instruments();

	// HACK LOADING
	const char *filename = NULL;
	if (argc > 1) filename = argv[1];
	if (filename) hack_load_track(filename);

	// Interrupt configuration.
	if (!interrupts_init()) goto done;

	// TODO: This should really be done based on an actual file and not test garbage

	// The interface starts in pattern mode, and we kick off the first draw.
	XtUiFocus focus = XT_UI_FOCUS_PATTERN;
	draw_fnlabels(focus, false);
	// The main loop.
	while (focus != XT_UI_FOCUS_QUIT)
	{
		xb_keys_poll();
		focus = editor_logic(focus);
		wait_for_vbl();
		editor_render(focus);
	}

done:
	interrupts_shutdown();
	display_config_shutdown();

	_dos_kflushio(0xFF);

	return 0;
}
