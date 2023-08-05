#include "xt.h"

#include <stdio.h>
#include <string.h>

#include "xbase/opm.h"

//
// OPM interaction functions
//

// Sets TL for a patch, accounting for the current amplitude value.
static inline void xt_set_opm_patch_tl(XtOpmChannelState *opm_state)
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
	XtOpmPatch *patch = &opm_state->patch;
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
				// the opposite fashion, so we inevrt it to get attenuation.i
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
static void xt_set_opm_patch_full(XtOpmChannelState *opm_state)
{
	XtOpmPatch *patch = &opm_state->patch;
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

static inline void xt_read_cell_cmd(Xt *xt, XtOpmChannelState *opm_state,
                                    uint8_t cmd, uint8_t arg)
{
	uint16_t opidx = 0;
	XtOpmPatch *patch = &opm_state->patch;
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

		case XT_CMD_AMPLITUDE:
			opm_state->amplitude = arg;
			break;

		// TODO: These will require some sort of register for which row / etc
		// is pending, so it can be enacted after all channels have run.
		case XT_CMD_BREAK:
		case XT_CMD_HALT:
		case XT_CMD_SKIP:
			break;

		case XT_CMD_SPEED:
			xt->current_ticks_per_row = arg;
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

		case XT_CMD_PORTAMENTO:
			opm_state->portamento_speed = (int8_t)arg;
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

		case XT_CMD_SLIDE_UP:
			// TODO: This
		case XT_CMD_SLIDE_DOWN:
			// TODO: This
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

static inline void xt_read_opm_cell_data(const Xt *xt, int16_t i,
                                     XtOpmChannelState *opm_state,
                                     const XtCell *cell)
{
	// NOTE: I'm explicitly checking for -1 here, because we may use -1 to
	// indicate no patch (note change without key event) in the future.
	if (opm_state->patch_no == -1 || cell->inst != opm_state->patch_no)
	{
		opm_state->patch_no = cell->inst;
		xt_set_opm_patch_full(opm_state);
	}
	opm_state->patch = xt->track.instruments[cell->inst].opm;

	if (cell->note == XT_NOTE_OFF)
	{
		opm_state->key_state = KEY_STATE_OFF;
		opm_state->key_command = KEY_COMMAND_OFF;
		opm_state->key_on_delay_count = 0;
		opm_state->patch_no = -1;
	}
	else if (cell->note == XT_NOTE_CUT)
	{
		opm_state->key_state = KEY_STATE_CUT;
		opm_state->key_command = KEY_COMMAND_OFF;
		opm_state->key_on_delay_count = 0;
		opm_state->patch_no = -1;
	}
	else
	{
		// convert {octave, note} to pitch number
		// effectively a multiply by 12 since we started *16
		uint16_t pnum = 3*((cell->note & XT_NOTE_OCTAVE_MASK) >> 2);
		// subtract off the offset applied when editing the note data
		pnum += (cell->note & XT_NOTE_TONE_MASK) - 1;
		opm_state->target_pitch = (pnum << 6) | opm_state->tune;
		opm_state->key_state = KEY_STATE_ON_PENDING;
		opm_state->key_on_delay_count = 0;
	}
}

// Read note and patch data from a cell.
static inline void xt_read_cell_data(const Xt *xt, int16_t i,
                                     XtOpmChannelState *opm_state,
                                     const XtCell *cell)
{
	// Update note, and set it up to be played.
	if (cell->note == XT_NOTE_NONE) return;

	switch (xt->chan[i].type)
	{
		default:
			return;
		case XT_CHANNEL_OPM:
			xt_read_opm_cell_data(xt, i, opm_state, cell);
			break;
	}
}

static inline void xt_update_key_state(XtOpmChannelState *opm_state)
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

static inline void xt_playback_counters(Xt *xt)
{
	xt->tick_counter++;
	if (xt->tick_counter >= xt->current_ticks_per_row)
	{
		xt->tick_counter = 0;
		xt->current_phrase_row++;
		if (xt->current_phrase_row >= xt->track.phrase_length)
		{
			xt->current_phrase_row = 0;
			if (!xt->repeat_frame) xt->current_frame++;
			if (xt->current_frame >= xt->track.num_frames)
			{
				if (xt->track.loop_point < 0)
				{
					xt->playing = false;
					xt->current_frame = 0;
				}
				else
				{
					xt->current_frame = xt->track.loop_point;
				}
			}
		}
	}
}

void channel_reset(XtChannelState *chan, int16_t voice)
{
	switch (chan->type)
	{
		default:
			break;
		case XT_CHANNEL_OPM:
			chan->opm.voice = voice;
			chan->opm.pan = OPM_PAN_BOTH;
			chan->opm.key_state = KEY_STATE_OFF;
			chan->opm.key_command = KEY_COMMAND_OFF;
			chan->opm.patch_no = -1;
			chan->opm.amplitude = 0x7F;
			xb_opm_set_key_on(chan->opm.voice, 0x0);
			break;
	}
}

void xt_init(Xt *xt)
{
	memset(xt, 0, sizeof(*xt));

	// Set default settings.
	xt->config.row_highlight[0] = 4;
	xt->config.row_highlight[1] = 16;

	// Populate pitch table.
	uint16_t pnum = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(xt->pitch_table); i++)
	{
		xt->pitch_table[i] = pnum++;
		if ((pnum & 3) == 3) pnum++;
	}

	// Set channel types.
	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		xt->chan[i].type = (i < XB_OPM_VOICE_COUNT) ? XT_CHANNEL_OPM : XT_CHANNEL_ADPCM;
		channel_reset(&xt->chan[i], i);
	}

	// TODO: Init/allocate channels and their types based on the track info.

}

void xt_poll_opm(Xt *xt, int16_t i, XtOpmChannelState *opm_state)
{
	const XtTrackChannelData *channel_data = &xt->track.channel_data[i];
	const uint16_t phrase_idx = xt->track.frames[xt->current_frame].phrase_id[i];
	const XtPhrase *phrase = &channel_data->phrases[phrase_idx];

	// The current cell.
	const XtCell *cell = &phrase->cells[xt->current_phrase_row];

	if (xt->tick_counter == 0)
	{
		xt_read_cell_data(xt, i, opm_state, cell);
		for (uint16_t j = 0; j < ARRAYSIZE(cell->cmd); j++)
		{
			xt_read_cell_cmd(xt, opm_state, cell->cmd[j].cmd, cell->cmd[j].arg);
		}
	}

	xt_mod_tick(&opm_state->mod_vibrato);
	xt_mod_tick(&opm_state->mod_tremolo);

	if (opm_state->portamento_speed == 0)
	{
		opm_state->current_pitch = opm_state->target_pitch;
	}
	else
	{
		if (opm_state->current_pitch < opm_state->target_pitch)
		{
			opm_state->current_pitch += opm_state->portamento_speed;
			if (opm_state->current_pitch > opm_state->target_pitch)
				opm_state->current_pitch = opm_state->target_pitch;
		}
		else if (opm_state->current_pitch > opm_state->target_pitch)
		{
			opm_state->current_pitch -= opm_state->portamento_speed;
			if (opm_state->current_pitch < opm_state->target_pitch)
				opm_state->current_pitch = opm_state->target_pitch;
		}
	}

	// compensate for 4MHz OPM
	int16_t pnum = (int16_t)opm_state->current_pitch + OPM_CLOCK_ADJUST;

	// compensate for note offset
	pnum -= (1 << 6);
	if (pnum < 0) pnum = 0;

	uint8_t kf = pnum << 2;
	uint8_t kc_idx = pnum >> 6;
	if (kc_idx >= ARRAYSIZE(xt->pitch_table))
	{
		kc_idx = ARRAYSIZE(xt->pitch_table) - 1;
		kf = 0xFC; // max out KF.
	}
	uint8_t kc = xt->pitch_table[kc_idx];
	opm_state->reg_kf_data = kf;
	opm_state->reg_kc_data = kc;

	xb_opm_set_kc(opm_state->voice, opm_state->reg_kc_data);
	xb_opm_set_key_fraction(opm_state->voice, opm_state->reg_kf_data);

	// TODO: Add vibrato to the pitch register caches (not applied to the
	// pitch data itself)

	xt_update_key_state(opm_state);
}

void xt_poll(Xt *xt)
{
	if (!xt->playing) return;
	// Process all channels for playback.
	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		XtChannelState *chan = &xt->chan[i];
		switch (chan->type)
		{
			default:
				break;

			case XT_CHANNEL_OPM:
				xt_poll_opm(xt, i, &chan->opm);
				break;

			case XT_CHANNEL_ADPCM:
				// TODO
				break;
		}
	}

	xt_playback_counters(xt);
}

void xt_update_opm_registers(Xt *xt)
{
	if (!xt->playing) return;
	// Commit registers based on new state.
	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		if (xt->chan[i].type != XT_CHANNEL_OPM) continue;
		XtOpmChannelState *opm_state = &xt->chan[i].opm;
		
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
	}
	xb_opm_commit();
}

void xt_start_playing(Xt *xt, int16_t frame, uint16_t repeat)
{
	for (uint16_t i = 0; i < XB_OPM_VOICE_COUNT; i++)
	{
		xb_opm_set_key_on(i, 0);
	}

	xb_opm_commit();
	xt->repeat_frame = repeat;
	xt->playing = true;
	xt->current_ticks_per_row = xt->track.ticks_per_row;
	xt->tick_counter = 0;

	if (frame >= 0) xt->current_frame = frame;
	if (xt->current_frame >= xt->track.num_frames)
	{
		xt->current_frame = xt->track.num_frames - 1;
	}
	xt->current_phrase_row = 0;
	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		channel_reset(&xt->chan[i], i);
	}
}

void xt_stop_playing(Xt *xt)
{
	cut_all_opm_sound();
	xt->playing = false;
}
