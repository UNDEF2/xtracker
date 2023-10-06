#include "xt/player.h"

#include <iocs.h>
#include <stdio.h>
#include <string.h>
#include "util/cgprint.h"
#include "xbase/ipl.h"
#include "xbase/opm.h"
#include "util/transpose.h"

//
// OPM interaction
//

// Pitch table for OPM frequencies calculated at init time.
static uint8_t s_pitch_table[8 * 12];

// Sets TL for a patch, accounting for the current amplitude value.
static inline void xt_set_opm_patch_tl(volatile XtChannelStateOpm *opm_state)
{
	// Defines for each algorithm which operators are carriers.
	const bool carrier_tbl[4 * 8] =
	{
		//Algo  Op 1    Op 3    Op 2    Op 4
		/* 0 */ false,  false,  false,  true,
		/* 1 */ false,  false,  false,  true,
		/* 2 */ false,  false,  false,  true,
		/* 3 */ false,  false,  false,  true,
		/* 4 */ false,  false,  true,   true,
		/* 5 */ false,  true,   true,   true,
		/* 6 */ false,  true,   true,   true,
		/* 7 */ true,   true,   true,   true,
	};
	volatile XtInstrumentOpm *patch = &opm_state->patch;
	const uint8_t i = opm_state->voice;

	for (uint16_t j = 0; j < XB_OPM_OP_COUNT; j++)
	{
		// Only carriers multiply the amplitude.
		if (carrier_tbl[(4 * patch->con) + j])
		{
			if (opm_state->key_state == KEY_STATE_CUT)
			{
				xb_opm_set_tl(i, j, 0x7F); // Silence.
			}
			else
			{
				// Amplitude is 0x7F (loud) to 0x00 (quiet), but OPM TL operates in
				// the opposite fashion, so we invet it to get attenuation.
				const int16_t tl_vol = 0x80 - patch->tl[j];
				const int16_t adj_tl_vol = (tl_vol * opm_state->amplitude) >> 7;
				const int16_t final_tl = 0x7F - adj_tl_vol;
				xb_opm_set_tl(i, j, final_tl);
			}
		}
		else
		{
			xb_opm_set_tl(i, j, patch->tl[j]);
		}
	}
}

// Sets the whole patch.
static void xt_set_opm_patch_full(volatile XtChannelStateOpm *opm_state)
{
	volatile XtInstrumentOpm *patch = &opm_state->patch;
	const uint8_t i = opm_state->voice;
	xb_opm_set_lr_fl_con(i, opm_state->pan, patch->fl, patch->con);
	xb_opm_set_pms_ams(i, patch->pms, patch->ams);
	for (uint16_t j = 0; j < XB_OPM_OP_COUNT; j++)
	{
		xb_opm_set_dt1_mul(i, j, patch->dt1[j], patch->mul[j]);
		xb_opm_set_ks_ar(i, j, patch->ks[j], patch->ar[j]);
		xb_opm_set_ame_d1r(i, j, patch->ame[j], patch->d1r[j]);
		xb_opm_set_dt2_d2r(i, j, patch->dt2[j], patch->d2r[j]);
		xb_opm_set_d1l_rr(i, j, patch->d1l[j], patch->rr[j]);
	}
	xt_set_opm_patch_tl(opm_state);
}

//
// Engine Functions
//

// Sets target pitch data based on a note value.
static inline void xt_set_note_pitch_data(volatile XtChannelStateOpm *opm_state,
                                          XtNote note)
{
	// convert {octave, note} to pitch number
	// effectively a multiply by 12 since we started *16
	uint16_t pnum = 3 * ((note & XT_NOTE_OCTAVE_MASK) >> 2);
	// subtract off the offset applied when editing the note data
	pnum += (note & XT_NOTE_TONE_MASK) - 1;
	opm_state->note = note & XT_NOTE_TONE_MASK;
	opm_state->target_pitch = (pnum << 6) | opm_state->tune;
	if (opm_state->current_pitch == 0)
	{
		opm_state->current_pitch = opm_state->target_pitch;
	}
}

// Command processing
static inline void read_cell_cmd_opm(volatile XtPlayer *xt, volatile XtChannelStateOpm *opm_state,
                                    uint8_t cmd, uint8_t arg)
{
	uint16_t opidx = 0;
	volatile XtInstrumentOpm *patch = &opm_state->patch;
	if (cmd == XT_CMD_NONE) return;
	switch (cmd)
	{
		default:
			// TODO: Print an error, probably
			break;

		case XT_CMD_TL_OP0:
		case XT_CMD_TL_OP1:
		case XT_CMD_TL_OP2:
		case XT_CMD_TL_OP3:
			opidx = cmd - XT_CMD_TL_OP0;
			opm_state->patch.tl[opidx] = arg;
			xb_opm_set_tl(opm_state->voice, opidx, arg);
			break;

		case XT_CMD_MULT_OP0:
		case XT_CMD_MULT_OP1:
		case XT_CMD_MULT_OP2:
		case XT_CMD_MULT_OP3:
			opidx = cmd - XT_CMD_MULT_OP0;
			opm_state->patch.mul[opidx] = arg;
			xb_opm_set_dt1_mul(opm_state->voice, opidx, patch->dt1[opidx], arg);
			break;

		// TODO: These will require some sort of register for which row / etc
		// is pending, so it can be enacted after all channels have run.
		case XT_CMD_BREAK:
			xt->pending_break_row = arg;

			if (xt->pending_break_row >= xt->track->meta.phrase_length)
			{
				xt->pending_break_row = -1;
			}
			break;

		case XT_CMD_GROOVE0:
			xt->groove[0] = arg;
			break;
		case XT_CMD_GROOVE1:
			xt->groove[1] = arg;
			break;

		case XT_CMD_NOISE_EN:
			xt->noise_enable = arg ? true : false;
			break;

		case XT_CMD_PAN:
			if (arg == 0x11) opm_state->pan = OPM_PAN_BOTH;
			else if (arg == 0x01) opm_state->pan = OPM_PAN_RIGHT;
			else if (arg == 0x10) opm_state->pan = OPM_PAN_LEFT;
			else opm_state->pan = OPM_PAN_NONE;
			xb_opm_set_lr_fl_con(opm_state->voice, opm_state->pan, patch->fl, patch->con);
			break;

		case XT_CMD_PERIOD:
			xt->timer_period = arg;
			xb_opm_set_clka_period(xt->timer_period);
			break;

		case XT_CMD_VIBRATO:
			opm_state->mod_vibrato.intensity = arg & 0x0F;
			opm_state->mod_vibrato.speed = (arg >> 4);
			break;
		case XT_CMD_VIBRATO_TYPE:
			opm_state->mod_vibrato.wave_type = arg;
			break;
		case XT_CMD_TREMOLO:
			opm_state->mod_tremolo.intensity = arg & 0x0F;
			opm_state->mod_tremolo.speed = (arg >> 4);
			break;
		case XT_CMD_TREMOLO_TYPE:
			opm_state->mod_vibrato.wave_type = arg;
			break;
			
		case XT_CMD_SLIDE:
			opm_state->mod_vibrato.intensity = 0;
			opm_state->mod_vibrato.speed = 0;
			opm_state->slide_speed = arg;
			opm_state->disable_slide_once_reached = false;
			break;


		case XT_CMD_SLIDE_UP:
			opm_state->mod_vibrato.intensity = 0;
			opm_state->mod_vibrato.speed = 0;
			opm_state->slide_speed = (arg & 0xF0) >> 4;
			if (opm_state->slide_speed != 0)
			{
				opm_state->note = xt_transpose_note(opm_state->note, (arg & 0x0F));
				xt_set_note_pitch_data(opm_state, opm_state->note);
				opm_state->disable_slide_once_reached = true;
			}
			break;

		case XT_CMD_SLIDE_DOWN:
			opm_state->mod_vibrato.intensity = 0;
			opm_state->mod_vibrato.speed = 0;
			opm_state->slide_speed = (arg & 0xF0) >> 4;
			if (opm_state->slide_speed != 0)
			{
				opm_state->note = xt_transpose_note(opm_state->note, (arg & 0x0F));
				xt_set_note_pitch_data(opm_state, opm_state->note);
				opm_state->disable_slide_once_reached = true;
			}
			break;
		case XT_CMD_MUTE_DELAY:
			opm_state->mute_delay_count = arg;
			break;
		case XT_CMD_NOTE_DELAY:
			opm_state->key_on_delay_count = arg;
			break;
		case XT_CMD_CUT_DELAY:
			opm_state->cut_delay_count = arg;
			break;

		case XT_CMD_TUNE:
			opm_state->tune = arg;
			break;
	}
}

static inline void xt_note_off_reset(volatile XtChannelStateOpm *opm_state)
{
	opm_state->key_command = KEY_COMMAND_OFF;
	opm_state->key_on_delay_count = 0;
	opm_state->patch_no = -1;
	opm_state->current_pitch = 0;
}

static inline void read_vol_data_opm(int16_t i,
                                     volatile XtChannelStateOpm *opm_state,
                                     const XtCell *cell)
{
	if (cell->vol < 0x80) return;
	const uint8_t actual_vol = cell->vol & 0x7F;
	opm_state->amplitude = actual_vol;
}

static inline void read_note_data_opm(const volatile XtPlayer *xt, int16_t i,
                                      volatile XtChannelStateOpm *opm_state,
                                      const XtCell *cell)
{
	if (cell->note == XT_NOTE_NONE) return;
	// NOTE: I'm explicitly checking for -1 here, because we may use -1 to
	// indicate no patch (note change without key event) in the future.
	if (opm_state->patch_no == -1 || cell->inst != opm_state->patch_no)
	{
		opm_state->patch_no = cell->inst;
		opm_state->patch = xt->track->instruments[cell->inst].opm;
		xt_set_opm_patch_full(opm_state);
	}

	if (cell->note == XT_NOTE_OFF)
	{
		opm_state->key_state = KEY_STATE_OFF;
		xt_note_off_reset(opm_state);
	}
	else if (cell->note == XT_NOTE_CUT)
	{
		opm_state->key_state = KEY_STATE_CUT;
		xt_note_off_reset(opm_state);
	}
	else
	{
		opm_state->current_pitch = 0;
		xt_set_note_pitch_data(opm_state, cell->note);
		opm_state->key_state = KEY_STATE_ON_PENDING;
		opm_state->key_on_delay_count = 0;
	}
}

static inline void xt_player_update_key_state(volatile XtChannelStateOpm *opm_state)
{
	if (opm_state->key_on_delay_count > 0)
	{
		opm_state->key_on_delay_count--;
	}

	if (opm_state->key_on_delay_count == 0 &&
		opm_state->key_state == KEY_STATE_ON_PENDING)
	{
		opm_state->key_on_delay_count = 0;
		opm_state->key_state = KEY_STATE_ON;
		opm_state->key_command = KEY_COMMAND_ON;
	}

	if (opm_state->mute_delay_count > 0)
	{
		opm_state->mute_delay_count--;
		if (opm_state->mute_delay_count == 0)
		{
			opm_state->key_state = KEY_STATE_OFF;
			opm_state->key_command = KEY_COMMAND_OFF;
		}
	}

	if (opm_state->cut_delay_count > 0)
	{
		opm_state->cut_delay_count--;
		if (opm_state->cut_delay_count == 0)
		{
			opm_state->key_state = KEY_STATE_CUT;
			opm_state->key_command = KEY_COMMAND_OFF;
		}
	}
}

static inline void xt_playback_counters(volatile XtPlayer *xt)
{
	xt->tick_counter++;
	if (xt->tick_counter >= xt->groove[xt->current_phrase_row % 2])
	{
		xt->tick_counter = 0;
		xt->current_phrase_row++;
		if (xt->current_phrase_row >= xt->track->meta.phrase_length ||
		    xt->pending_break_row >= 0)
		{
			if (xt->pending_break_row >= 0)
			{
				xt->current_phrase_row = xt->pending_break_row;
				xt->pending_break_row = -1;
			}
			else
			{
				xt->current_phrase_row = 0;
			}
			if (!xt->repeat_frame)
			{
				xt->current_frame++;
			}
			if (xt->current_frame >= xt->track->num_frames)
			{
				if (xt->track->meta.loop_point < 0)
				{
					xt->playing = false;
					xt->current_frame = 0;
				}
				else
				{
					xt->current_frame = xt->track->meta.loop_point;
				}
			}
			
		}
	}
}

void channel_reset(volatile XtChannelState *chan, int16_t voice)
{
	switch (chan->type)
	{
		default:
			break;
		case XT_INSTRUMENT_TYPE_OPM:
			chan->opm.voice = voice;
			chan->opm.pan = OPM_PAN_BOTH;
			chan->opm.key_state = KEY_STATE_OFF;
			chan->opm.key_command = KEY_COMMAND_OFF;
			chan->opm.patch_no = -1;
			chan->opm.amplitude = 0x7F;
			chan->opm.key_on_delay_count = 0;
			chan->opm.mute_delay_count = 0;
			chan->opm.cut_delay_count = 0;
			chan->opm.mod_vibrato.intensity = 0;
			chan->opm.mod_vibrato.speed = 0;
			xb_opm_set_key_on(chan->opm.voice, 0x0);

			break;
	}
}

void xt_player_init(volatile XtPlayer *xt, const XtTrack *track)
{
	// Not using memset because there isn't a memset compatible with volatile
	// semantics available to us.
	volatile uint8_t *xt_u8 = (volatile uint8_t *)xt;
	for (size_t i = 0; i < sizeof(*xt); i++) xt_u8[i] = 0;
	xt->track = track;

	// Populate pitch table.
	uint16_t pnum = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(s_pitch_table); i++)
	{
		s_pitch_table[i] = pnum++;
		if ((pnum & 3) == 3) pnum++;
	}

	// Set up channels based on track data.
	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		xt->chan[i].type = track->channel_data[i].type;
		channel_reset(&xt->chan[i], i);
	}
}

void xt_player_poll_opm(volatile XtPlayer *xt, int16_t i, volatile XtChannelStateOpm *opm_state)
{
	const XtTrackChannelData *channel_data = &xt->track->channel_data[i];
	const uint16_t phrase_idx = xt->track->frames[xt->current_frame].phrase_id[i];
	const XtPhrase *phrase = &channel_data->phrases[phrase_idx];

	// The current cell.
	const XtCell *cell = &phrase->cells[xt->current_phrase_row];

	if (xt->tick_counter == 0)
	{
		read_note_data_opm(xt, i, opm_state, cell);
		read_vol_data_opm(i, opm_state, cell);
		for (uint16_t j = 0; j < ARRAYSIZE(cell->cmd); j++)
		{
			read_cell_cmd_opm(xt, opm_state, cell->cmd[j].cmd, cell->cmd[j].arg);
		}
	}

	// Sliding
	if (opm_state->key_state == KEY_STATE_ON)
	{
		if (opm_state->slide_speed == 0)
		{
			opm_state->current_pitch = opm_state->target_pitch;
		}
		else
		{
			if (opm_state->current_pitch < opm_state->target_pitch)
			{
				opm_state->current_pitch += opm_state->slide_speed;
				if (opm_state->current_pitch >= opm_state->target_pitch)
				{
					if (opm_state->disable_slide_once_reached) opm_state->slide_speed = 0;
					opm_state->current_pitch = opm_state->target_pitch;
				}
			}
			else if (opm_state->current_pitch > opm_state->target_pitch)
			{
				opm_state->current_pitch -= opm_state->slide_speed;
				if (opm_state->current_pitch <= opm_state->target_pitch)
				{
					if (opm_state->disable_slide_once_reached) opm_state->slide_speed = 0;
					opm_state->current_pitch = opm_state->target_pitch;
				}
			}
		}
	}

	xt_mod_tick(&opm_state->mod_tremolo);
	// TODO: Apply tremolo output

	// compensate for 4MHz OPM
	int16_t pnum = (int16_t)opm_state->current_pitch + OPM_CLOCK_ADJUST;

	if (opm_state->mod_vibrato.intensity > 0)
	{
		xt_mod_tick(&opm_state->mod_vibrato);
		pnum += opm_state->mod_vibrato.value;
	}

	// compensate for note offset
	pnum -= (1 << 6);
	if (pnum < 0) pnum = 0;

	uint8_t kf = pnum << 2;
	uint8_t kc_idx = pnum >> 6;
	if (kc_idx >= ARRAYSIZE(s_pitch_table))
	{
		kc_idx = ARRAYSIZE(s_pitch_table) - 1;
		kf = 0xFC; // max out KF.
	}
	uint8_t kc = s_pitch_table[kc_idx];
	opm_state->reg_kf_data = kf;
	opm_state->reg_kc_data = kc;

	xb_opm_set_kc(opm_state->voice, opm_state->reg_kc_data);
	xb_opm_set_key_fraction(opm_state->voice, opm_state->reg_kf_data);

	if (xt->noise_enable && opm_state->voice == 7)
	{
		// TODO: Figure out how noise works and set it properly
		xb_opm_set_noise(true, kc);
	}

	// TODO: Add vibrato to the pitch register caches (not applied to the
	// pitch data itself)

	xt_player_update_key_state(opm_state);

//	_iocs_ledmod(opm_state->voice, opm_state->key_state == KEY_STATE_ON);
}

void xt_player_poll(volatile XtPlayer *xt)
{
	if (!xt->playing) return;
	cgbox(XT_UI_PLANE, 0, 8, 92, 64, 64);
	// Process all channels for playback.
	for (uint16_t i = 0; i < 8; i++)  // TODO: Make sure channel type is set right!!! ARRAYSIZE(xt->chan); i++)
	{
		volatile XtChannelState *chan = &xt->chan[i];
		switch (chan->type)
		{
			default:
				break;

			case XT_INSTRUMENT_TYPE_OPM:
				xt_player_poll_opm(xt, i, &chan->opm);
				break;

			case XT_INSTRUMENT_TYPE_MSM6258:
				// TODO
				break;
		}
	}

	xt_playback_counters(xt);
}

void xt_player_update_opm_registers(volatile XtPlayer *xt)
{
	const uint8_t old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	// Commit registers based on new state.
	for (uint16_t i = 0; i < 8; i++)
	{
		if (xt->chan[i].type != XT_INSTRUMENT_TYPE_OPM) continue;
		volatile XtChannelStateOpm *opm_state = &xt->chan[i].opm;
		xt_set_opm_patch_tl(opm_state);

		// Key state
		if (opm_state->key_command == KEY_COMMAND_ON)
		{
			xb_opm_set_key_on(i, 0x0);
			xb_opm_set_key_on(i, 0xF);
			opm_state->key_command = KEY_COMMAND_NONE;
		}
		else if (opm_state->key_command == KEY_COMMAND_OFF)
		{
			xb_opm_set_key_on(i, 0x0);
			opm_state->key_command = KEY_COMMAND_NONE;
		}
	}

	xb_opm_commit();
	xb_set_ipl(old_ipl);
}

static inline void cut_all_opm_sound(void)
{
	// Silence any lingering channel noise.
	for (uint16_t i = 0; i < XB_OPM_VOICE_COUNT; i++)
	{
		xb_opm_set_key_on(i, 0);
		for (uint16_t j = 0; j < XB_OPM_OP_COUNT; j++)
		{
			xb_opm_set_tl(i, j, 0x7F);
		}
		//_iocs_ledmod(i, 0);
	}
	xb_opm_commit();
}

void xt_player_start_playing(volatile XtPlayer *xt, int16_t frame, bool repeat_frame)
{
	const uint8_t s_old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	cut_all_opm_sound();

	xt->current_frame = frame;
	xt->repeat_frame = repeat_frame;

	xt->current_phrase_row = 0;
	xt->tick_counter = 0;
	xt->noise_enable = false;
	xt->pending_break_row = -1;

	// Initial state that comes from the track.
	xt->groove[0] = xt->track->meta.groove[0];
	xt->groove[1] = xt->track->meta.groove[1];
	xt->timer_period = xt->track->meta.timer_period;
	xb_opm_set_clka_period(xt->timer_period);

	// Limit an unreasonable frame request
	if (xt->current_frame >= xt->track->num_frames)
	{
		xt->current_frame = xt->track->num_frames - 1;
	}

	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		channel_reset(&xt->chan[i], i);
	}
	xb_opm_set_noise(false, 0);

	xt->playing = true;
	xb_set_ipl(s_old_ipl);
}

void xt_player_stop_playing(volatile XtPlayer *xt)
{
	const uint8_t s_old_ipl = xb_set_ipl(XB_IPL_ALLOW_NONE);
	cut_all_opm_sound();
	xt->playing = false;
	xb_set_ipl(s_old_ipl);
}

bool xt_player_is_playing(const volatile XtPlayer *xt)
{
	return xt->playing;
}

void xt_player_get_playback_pos(const volatile XtPlayer *xt, volatile int16_t *frame, volatile int16_t *row)
{
	if (frame) *frame = xt->current_frame;
	if (row) *row = xt->current_phrase_row;
}
