#include "xt.h"

#include <stdio.h>
#include <string.h>

#include "xbase/opm.h"

static inline void xt_read_cell_cmd(Xt *xt, XtOpmChannelState *opm_state,
                                    uint8_t cmd, uint8_t arg)
{
	if (cmd == XT_CMD_NONE) return;
	switch (cmd)
	{
		default:
			// TODO: Print an error, probably
			break;

		case XT_CMD_TL_OP0:
			opm_state->patch.tl[0] = 0x7F;
			break;
		case XT_CMD_TL_OP1:
			opm_state->patch.tl[1] = 0x7F;
			break;
		case XT_CMD_TL_OP2:
			opm_state->patch.tl[2] = 0x7F;
			break;
		case XT_CMD_TL_OP3:
			opm_state->patch.tl[3] = 0x7F;
			break;

		case XT_CMD_MULT_OP0:
			opm_state->patch.mul[0] = arg;
			break;
		case XT_CMD_MULT_OP1:
			opm_state->patch.mul[1] = arg;
			break;
		case XT_CMD_MULT_OP2:
			opm_state->patch.mul[2] = arg;
			break;
		case XT_CMD_MULT_OP3:
			opm_state->patch.mul[3] = arg;
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
			if (arg == 0x01) opm_state->pan = OPM_PAN_RIGHT;
			if (arg == 0x10) opm_state->pan = OPM_PAN_LEFT;
			else opm_state->pan = OPM_PAN_NONE;
			break;

		case XT_CMD_TUNE:
			opm_state->tune = arg;
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
	}
}

static inline void xt_read_opm_cell_data(const Xt *xt, int16_t i,
                                     XtOpmChannelState *opm_state,
                                     const XtCell *cell)
{
	if (xt->track.instruments[cell->inst].type != XT_CHANNEL_OPM) return;
	opm_state->patch = xt->track.instruments[cell->inst].opm;


	if (cell->note == XT_NOTE_OFF)
	{
		opm_state->key_state = KEY_STATE_OFF;
		opm_state->key_command = KEY_COMMAND_OFF;
		opm_state->key_on_delay_count = 0;
	}
	else if (cell->note == XT_NOTE_CUT)
	{
		opm_state->key_state = KEY_STATE_CUT;
		opm_state->key_command = KEY_COMMAND_OFF;
		opm_state->key_on_delay_count = 0;
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
		xt->chan[i].type = (i < 8) ? XT_CHANNEL_OPM : XT_CHANNEL_ADPCM;
		xt->chan[i].opm.pan = OPM_PAN_BOTH;
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
		// TODO: Slide towards target pitch at portamento speed.
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
		XtOpmPatch *patch = &opm_state->patch;

		// Key state
		if (opm_state->key_command == KEY_COMMAND_ON)
		{
			xb_opm_set_key_on(i, 0x0);
			xb_opm_set_key_on(i, 0xF);
			opm_state->key_command = KEY_COMMAND_NONE;
		}
		else if (opm_state->key_command == KEY_COMMAND_OFF)
		{
			// TODO: Remove once we have TL cache
			if (opm_state->key_state == KEY_STATE_CUT)
			{
				memset(patch->tl, 0x7F, sizeof(patch->tl));
			}
			xb_opm_set_key_on(i, 0x0);
			opm_state->key_command = KEY_COMMAND_NONE;
		}

		// TODO: Create TL caches, and use that to apply both note cut, and
		//       the channel amplitude settings, to the TL.

		xb_opm_set_kc(i, opm_state->reg_kc_data);
		xb_opm_set_key_fraction(i, opm_state->reg_kf_data);
		xb_opm_set_lr_fl_con(i, opm_state->pan, patch->fl, patch->con);
		xb_opm_set_pms_ams(i, patch->pms, patch->ams);
		for (uint16_t j = 0; j < XB_OPM_OP_COUNT; j++)
		{
			xb_opm_set_dt1_mul(i, j, patch->dt1[j], patch->mul[j]);
			xb_opm_set_tl(i, j, patch->tl[j]);
			xb_opm_set_ks_ar(i, j, patch->ks[j], patch->ar[j]);
			xb_opm_set_ame_d1r(i, j, patch->ame[j], patch->d1r[j]);
			xb_opm_set_dt2_d2r(i, j, patch->dt2[j], patch->d2r[j]);
			xb_opm_set_d1l_rr(i, j, patch->d1l[j], patch->rr[j]);
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
	cut_all_opm_sound();
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
		XtChannelState *chan = &xt->chan[i];
		chan->opm.pan = OPM_PAN_BOTH;
	}
}

void xt_stop_playing(Xt *xt)
{
	cut_all_opm_sound();
	xt->playing = false;
}
