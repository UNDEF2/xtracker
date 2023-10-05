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

#include "core/macro.h"
#include "edit/phrase_editor.h"
#include "edit/arrange_editor.h"
#include "edit/instrument_editor.h"
#include "core/display_config.h"
#include "util/cgprint.h"
#include "ui/arrange_render.h"
#include "ui/backing.h"
#include "ui/cursor.h"
#include "ui/chanlabels.h"
#include "ui/fnlabels.h"
#include "ui/instrument_list_render.h"
#include "ui/metrics.h"
#include "ui/regdata_render.h"
#include "ui/track_render.h"

#include "palette.h"
#include "xt/player.h"
#include "xt/instrument.h"

//
// Editor / Interface state
//
static XtTrack s_track;
static XtArrangeRenderer s_arrange_renderer;
static XtRegdataRenderer s_regdata_renderer;
static XtTrackRenderer s_track_renderer;
static XtPhraseEditor s_phrase_editor;
static XtArrangeEditor s_arrange_editor;
static XtInstrumentListRenderer s_instrument_list_renderer;
static XtInstrumentEditor s_instrument_editor;

static const char *s_filename;

//
// Player (driven by OPM timer interrupt)
//

static volatile XtPlayer s_player;

// Copies of last player status, updated in the OPM ISR during playback.
static volatile int16_t s_player_frame;
static volatile int16_t s_player_row;

static void XB_ISR s_isr_opm(void)
{
	xt_player_poll(&s_player);
	xt_player_update_opm_registers(&s_player);
	xt_player_get_playback_pos(&s_player, &s_player_frame, &s_player_row);
	xb_opm_set_timer_flags(OPM_TIMER_FLAG_IRQ_EN_A | OPM_TIMER_FLAG_LOAD_A | OPM_TIMER_FLAG_F_RESET_A);
}

//
// Interrupt management (VBL and original IPL storage)
//
static uint8_t s_old_ipl;
static void (*s_old_vbl_isr)(void);
static volatile bool s_vbl_hit;

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
	memset(s_track.instruments, 0, sizeof(s_track.instruments[0]) * 5);
	XtInstrument *ins = &s_track.instruments[0];

	// Instrument $00
	ins->type = XT_CHANNEL_OPM;
	snprintf(ins->name, sizeof(ins->name), "Bass");

	// In furnace UI order
	ins->opm.fl = 1;
	ins->opm.con = 0;
	ins->opm.pms = 0;
	ins->opm.ams = 0;

	ins->opm.ar [0] = 31; // AKA A
	ins->opm.d1r[0] = 18; // AKA D
	ins->opm.d1l[0] = 2;  // AKA S
	ins->opm.d2r[0] = 0;  // AKA D2
	ins->opm.rr [0] = 15;  // AKA R
	ins->opm.tl [0] = 36;
	ins->opm.ks [0] = 0;  // AKA RS
	ins->opm.mul[0] = 10;  // AKA ML
	ins->opm.dt1[0] = 0;  // AKA DT
	ins->opm.dt2[0] = 0;  // AKA DT2
	ins->opm.ame[0] = 0;  // AKA AM

	ins->opm.ar [2] = 31; // AKA A
	ins->opm.d1r[2] = 14; // AKA D
	ins->opm.d1l[2] = 2;  // AKA S
	ins->opm.d2r[2] = 4;  // AKA D2
	ins->opm.rr [2] = 15;  // AKA R
	ins->opm.tl [2] = 45;
	ins->opm.ks [2] = 0;  // AKA RS
	ins->opm.mul[2] = 0;  // AKA ML
	ins->opm.dt1[2] = 0x100 - 3;  // AKA DT
	ins->opm.dt2[2] = 0;  // AKA DT2
	ins->opm.ame[2] = 0;  // AKA AM

	ins->opm.ar [1] = 31; // AKA A
	ins->opm.d1r[1] = 10; // AKA D
	ins->opm.d1l[1] = 2;  // AKA S
	ins->opm.d2r[1] = 4;  // AKA D2
	ins->opm.rr [1] = 15;  // AKA R
	ins->opm.tl [1] = 19;
	ins->opm.ks [1] = 1;  // AKA RS
	ins->opm.mul[1] = 0;  // AKA ML
	ins->opm.dt1[1] = 3;  // AKA DT
	ins->opm.dt2[1] = 0;  // AKA DT2
	ins->opm.ame[1] = 0;  // AKA AM

	ins->opm.ar [3] = 31; // AKA A
	ins->opm.d1r[3] = 5; // AKA D
	ins->opm.d1l[3] = 0;  // AKA S
	ins->opm.d2r[3] = 2;  // AKA D2
	ins->opm.rr [3] = 15;  // AKA R
	ins->opm.tl [3] = 5;
	ins->opm.ks [3] = 1;  // AKA RS
	ins->opm.mul[3] = 0;  // AKA ML
	ins->opm.dt1[3] = 0; // AKA DT
	ins->opm.dt2[3] = 0;  // AKA DT2
	ins->opm.ame[3] = 0;  // AKA AM

	// instrument $01 - filled crap data
	ins++;
	uint8_t val = 0;
	ins->opm.con = val++;
	ins->opm.fl = val++;
	ins->opm.pms = val++;
	ins->opm.ams = val++;
	snprintf(ins->name, sizeof(ins->name), "Lead 02");

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

	ins++;
	snprintf(ins->name, sizeof(ins->name), "Sine");
	ins->opm.fl = 1;
	ins->opm.con = 0;
	ins->opm.pms = 0;
	ins->opm.ams = 0;
	ins->opm.ar [0] = 31; // AKA A
	ins->opm.d1r[0] = 18; // AKA D
	ins->opm.d1l[0] = 2;  // AKA S
	ins->opm.d2r[0] = 0;  // AKA D2
	ins->opm.rr [0] = 15;  // AKA R
	ins->opm.tl [0] = 127;
	ins->opm.ks [0] = 0;  // AKA RS
	ins->opm.mul[0] = 10;  // AKA ML
	ins->opm.dt1[0] = 0;  // AKA DT
	ins->opm.dt2[0] = 0;  // AKA DT2
	ins->opm.ame[0] = 0;  // AKA AM

	ins->opm.ar [2] = 31; // AKA A
	ins->opm.d1r[2] = 14; // AKA D
	ins->opm.d1l[2] = 2;  // AKA S
	ins->opm.d2r[2] = 4;  // AKA D2
	ins->opm.rr [2] = 15;  // AKA R
	ins->opm.tl [2] = 127;
	ins->opm.ks [2] = 0;  // AKA RS
	ins->opm.mul[2] = 0;  // AKA ML
	ins->opm.dt1[2] = 0x100 - 3;  // AKA DT
	ins->opm.dt2[2] = 0;  // AKA DT2
	ins->opm.ame[2] = 0;  // AKA AM

	ins->opm.ar [1] = 31; // AKA A
	ins->opm.d1r[1] = 10; // AKA D
	ins->opm.d1l[1] = 2;  // AKA S
	ins->opm.d2r[1] = 4;  // AKA D2
	ins->opm.rr [1] = 15;  // AKA R
	ins->opm.tl [1] = 127;
	ins->opm.ks [1] = 1;  // AKA RS
	ins->opm.mul[1] = 0;  // AKA ML
	ins->opm.dt1[1] = 3;  // AKA DT
	ins->opm.dt2[1] = 0;  // AKA DT2
	ins->opm.ame[1] = 0;  // AKA AM

	ins->opm.ar [3] = 31; // AKA A
	ins->opm.d1r[3] = 5; // AKA D
	ins->opm.d1l[3] = 0;  // AKA S
	ins->opm.d2r[3] = 2;  // AKA D2
	ins->opm.rr [3] = 15;  // AKA R
	ins->opm.tl [3] = 5;
	ins->opm.ks [3] = 1;  // AKA RS
	ins->opm.mul[3] = 1;  // AKA ML
	ins->opm.dt1[3] = 0; // AKA DT
	ins->opm.dt2[3] = 0;  // AKA DT2
	ins->opm.ame[3] = 0;  // AKA AM

	s_track.num_instruments = 3;
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

// Draws the channel labels above each column.
static void draw_chanlabels(int16_t cam_x)
{
	const int16_t left_visible_channel = cam_x / (XT_RENDER_CELL_W_PIXELS *
	                                              XT_RENDER_CELL_CHARS);
	for (int16_t i = 0; i < XT_RENDER_VISIBLE_CHANNELS; i++)
	{
		const int chan_idx = left_visible_channel + i;
		const XtChannelType type = s_track.channel_data[chan_idx].type;
		ui_chanlabel_set(i, chan_idx, type);
	}
}

// Updates the channel labels if the draw position has changed since last time.
static void maybe_set_chanlabels(int16_t cam_x)
{
	static int16_t s_last_cam_x = -1;
	if (s_last_cam_x != cam_x)
	{
		draw_chanlabels(cam_x);
		s_last_cam_x = cam_x;
	}
}

// Draws the function key labels at the bottom of the screen.
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

	static bool right_drawn = false;
	if (right_drawn) return;
	right_drawn = true;
	ui_fnlabel_set(5, "Arrange");
	ui_fnlabel_set(6, "Instr");
	ui_fnlabel_set(7, "Meta");
	ui_fnlabel_set(8, "Pattern");
	ui_fnlabel_set(9, "File");
}

// Updates the function key labels if the UI focus has changed.
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

// Starts or stops music playback.
static void toggle_playback(XBKeyEvent key_event)
{
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	if (key_event.modifiers & XB_KEY_MOD_KEY_UP) goto done;
	if (xt_player_is_playing(&s_player))
	{
		if (key_event.name == XB_KEY_CR)
		{
			xt_player_stop_playing(&s_player);
		}
	}
	else
	{
		if (key_event.name == XB_KEY_CR)
		{
			xt_player_start_playing(&s_player, s_phrase_editor.frame,
			                        xb_key_on(XB_KEY_SHIFT));
		}
		xt_phrase_editor_on_key(&s_phrase_editor, &s_track, key_event);
	}
done:
	xb_set_ipl(old_ipl);
}

static void update_scroll(void)
{
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	// Set camera / scroll for pattern field.
	// Focus the "camera" down a little bit to make room for the HUD.
	if (xt_player_is_playing(&s_player))
	{
		const int16_t yscroll = (s_player_row - 16) * 8;
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

// Draws the current UI focus if it has changed since last time.
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
	cgprint(XT_UI_PLANE, XT_PAL_MAIN, str,
	        XT_UI_FOCUS_LABEL_X, XT_UI_FOCUS_LABEL_Y);
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
			xt_instrument_list_renderer_tick(&s_instrument_list_renderer, &s_track,
			                                 s_phrase_editor.instrument);
			break;
		case XT_UI_FOCUS_ARRANGE_EDIT:
			xt_arrange_renderer_tick(&s_arrange_renderer, &s_track,
			                         s_arrange_editor.frame, s_arrange_editor.column);
			break;
		case XT_UI_FOCUS_INSTRUMENT_SEL:
			xt_regdata_renderer_enable_edit_cursor(&s_regdata_renderer, false);
			xt_regdata_renderer_tick(&s_regdata_renderer, &s_track.instruments[s_phrase_editor.instrument]);
			xt_instrument_list_renderer_tick(&s_instrument_list_renderer, &s_track,
			                                 s_phrase_editor.instrument);
			break;
		case XT_UI_FOCUS_INSTRUMENT_EDIT:
			xt_regdata_renderer_tick(&s_regdata_renderer, &s_track.instruments[s_phrase_editor.instrument]);
			xt_instrument_list_renderer_tick(&s_instrument_list_renderer, &s_track,
			                                 s_phrase_editor.instrument);
			break;
	}

	xt_cursor_update();
	update_scroll();

	// Update the track renderer to repaint pattern data.
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	const int16_t displayed_frame = (xt_player_is_playing(&s_player) ? s_player_frame : s_phrase_editor.frame);
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
	const bool playing = xt_player_is_playing(&s_player);
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
				focus = (focus == XT_UI_FOCUS_INSTRUMENT_SEL) ? XT_UI_FOCUS_INSTRUMENT_EDIT : XT_UI_FOCUS_INSTRUMENT_SEL;
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
				if (s_filename) xt_track_save_to_file(&s_track, s_filename);
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
				else
				{
					const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
					s_arrange_editor.frame = s_player_frame;
					xb_set_ipl(old_ipl);
				}
				break;

			case XT_UI_FOCUS_ARRANGE_EDIT:
				if (key_event.name == XB_KEY_ESC)
				{
					focus_on_pattern();
					focus = XT_UI_FOCUS_PATTERN;
				}
				else
				{
					if (!playing)
					{
						xt_arrange_editor_on_key(&s_arrange_editor,
						                         &s_track,
						                         &s_track_renderer, key_event);
						s_phrase_editor.frame = s_arrange_editor.frame;
						s_phrase_editor.column = s_arrange_editor.column;
					}
					else
					{
						const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
						s_arrange_editor.frame = s_player_frame;
						xb_set_ipl(old_ipl);
					}
				}
				break;

			case XT_UI_FOCUS_INSTRUMENT_SEL:
				if (key_event.name == XB_KEY_ESC)
				{
					focus_on_pattern();
					focus = XT_UI_FOCUS_PATTERN;
				}

				if (!(key_event.modifiers & XB_KEY_MOD_KEY_UP))
				{
					switch (key_event.name)
					{
						default:
							break;
						case XB_KEY_DOWN:
							s_phrase_editor.instrument++;
							if (s_phrase_editor.instrument >= s_track.num_instruments)
							{
								s_phrase_editor.instrument = s_track.num_instruments - 1;
							}
							break;

						case XB_KEY_UP:
							if (s_phrase_editor.instrument > 0) s_phrase_editor.instrument--;
							break;

						case XB_KEY_ESC:
							focus_on_pattern();
							focus = XT_UI_FOCUS_PATTERN;
							break;

						case XB_KEY_F5:
							focus = XT_UI_FOCUS_INSTRUMENT_EDIT;
							break;
					}

				}

				break;

			case XT_UI_FOCUS_INSTRUMENT_EDIT:
				xt_instrument_editor_on_key(&s_instrument_editor,
				                            s_phrase_editor.instrument,
				                            &s_track, key_event);
				if (key_event.name == XB_KEY_ESC)
				{
					focus = XT_UI_FOCUS_INSTRUMENT_SEL;
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
	xt_palette_init();
	xb_keys_init(NULL);

	// Interrupt configuration.
	if (!interrupts_init()) goto done;

	cgprint_load("RES\\CGDAT.BIN");

	xt_player_init(&s_player, &s_track);
	xt_cursor_init();
	xt_track_renderer_init(&s_track_renderer);
	xt_arrange_renderer_init(&s_arrange_renderer);
	xt_arrange_editor_init(&s_arrange_editor, &s_arrange_renderer);
	xt_phrase_editor_init(&s_phrase_editor, &s_track);
	xt_regdata_renderer_init(&s_regdata_renderer);
	xt_instrument_list_renderer_init(&s_instrument_list_renderer);
	xt_instrument_editor_init(&s_instrument_editor, &s_regdata_renderer);

	ui_backing_draw();

	// Track file load.
	bool file_loaded = false;
	if (argc > 1) s_filename = argv[1];
	if (s_filename) file_loaded = (xt_track_load_from_file(&s_track, s_filename) == XT_RES_OK);

	if (!file_loaded)
	{
		xt_track_init(&s_track);
		set_demo_instruments();
		if (s_filename) xt_track_save_to_file(&s_track, s_filename);
	}

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
