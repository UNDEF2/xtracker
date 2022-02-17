#include "xt.h"

#include <stdio.h>
#include <string.h>

#include "x68000/x68k_opm.h"

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
			opm_state->patch.dt1_mul[0] &= 0xF0;
			opm_state->patch.dt1_mul[0] |= arg & 0x0F;
			break;
		case XT_CMD_MULT_OP1:
			opm_state->patch.dt1_mul[1] &= 0xF0;
			opm_state->patch.dt1_mul[1] |= arg & 0x0F;
			break;
		case XT_CMD_MULT_OP2:
			opm_state->patch.dt1_mul[2] &= 0xF0;
			opm_state->patch.dt1_mul[2] |= arg & 0x0F;
			break;
		case XT_CMD_MULT_OP3:
			opm_state->patch.dt1_mul[3] &= 0xF0;
			opm_state->patch.dt1_mul[3] |= arg & 0x0F;
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
			xt->noise_enable = arg;
			break;

		case XT_CMD_PAN:
			if (arg == 0x11) opm_state->reg_20_overlay = 0xC0;
			else if (arg == 0x01) opm_state->reg_20_overlay = 0x80;
			else if (arg == 0x10) opm_state->reg_20_overlay = 0x40;
			else opm_state->reg_20_overlay = 0;
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

static inline void xt_update_opm_key_state(XtOpmChannelState *opm_state)
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
					xt->playing = 0;
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
	opm_state->reg_30_cache = kf;
	opm_state->reg_28_cache = kc;

	// TODO: Add vibrato to the pitch register caches (not applied to the
	// pitch data itself)

	xt_update_opm_key_state(opm_state);
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

static inline void opm_tx(uint8_t addr, uint8_t new_val, uint8_t old_val, uint8_t force)
{
	if (force || new_val != old_val) x68k_opm_write(addr, new_val);
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
		XtOpmPatch *patch_prev = &opm_state->patch_prev;

		if (opm_state->key_command == KEY_COMMAND_ON)
		{
			x68k_opm_set_key_on(i, 0x0);
			x68k_opm_set_key_on(i, 0xF);
			opm_state->key_command = KEY_COMMAND_NONE;
		}
		else if (opm_state->key_command == KEY_COMMAND_OFF)
		{
			// TODO: Remove once we have TL cache
			if (opm_state->key_state == KEY_STATE_CUT)
			{
				patch->tl[0] = 0x7F;
				patch->tl[1] = 0x7F;
				patch->tl[2] = 0x7F;
				patch->tl[3] = 0x7F;
			}
			x68k_opm_set_key_on(i, 0x0);
			opm_state->key_command = KEY_COMMAND_NONE;
		}

		// TODO: Create TL caches, and use that to apply both note cut, and
		//       the channel amplitude settings, to the TL.

		opm_tx(i + 0x28, opm_state->reg_28_cache, opm_state->reg_28_cache_prev, opm_state->cache_invalid);
		opm_tx(i + 0x30, opm_state->reg_30_cache, opm_state->reg_30_cache_prev, opm_state->cache_invalid);

		opm_tx(i + 0x20, patch->pan_fl_con | opm_state->reg_20_overlay,
		                patch_prev->pan_fl_con | opm_state->reg_20_overlay, 1);
		opm_tx(i + 0x38, patch->pms_ams, patch_prev->pms_ams, opm_state->cache_invalid);

		opm_tx(i + 0x40, patch->dt1_mul[0], patch_prev->dt1_mul[0], opm_state->cache_invalid);
		opm_tx(i + 0x48, patch->dt1_mul[1], patch_prev->dt1_mul[1], opm_state->cache_invalid);
		opm_tx(i + 0x50, patch->dt1_mul[2], patch_prev->dt1_mul[2], opm_state->cache_invalid);
		opm_tx(i + 0x58, patch->dt1_mul[3], patch_prev->dt1_mul[3], opm_state->cache_invalid);

		opm_tx(i + 0x60, patch->tl[0], patch_prev->tl[0], opm_state->cache_invalid);
		opm_tx(i + 0x68, patch->tl[1], patch_prev->tl[1], opm_state->cache_invalid);
		opm_tx(i + 0x70, patch->tl[2], patch_prev->tl[2], opm_state->cache_invalid);
		opm_tx(i + 0x78, patch->tl[3], patch_prev->tl[3], opm_state->cache_invalid);

		opm_tx(i + 0x80, patch->ks_ar[0], patch_prev->ks_ar[0], opm_state->cache_invalid);
		opm_tx(i + 0x88, patch->ks_ar[1], patch_prev->ks_ar[1], opm_state->cache_invalid);
		opm_tx(i + 0x90, patch->ks_ar[2], patch_prev->ks_ar[2], opm_state->cache_invalid);
		opm_tx(i + 0x98, patch->ks_ar[3], patch_prev->ks_ar[3], opm_state->cache_invalid);

		opm_tx(i + 0xA0, patch->ame_d1r[0], patch_prev->ame_d1r[0], opm_state->cache_invalid);
		opm_tx(i + 0xA8, patch->ame_d1r[1], patch_prev->ame_d1r[1], opm_state->cache_invalid);
		opm_tx(i + 0xB0, patch->ame_d1r[2], patch_prev->ame_d1r[2], opm_state->cache_invalid);
		opm_tx(i + 0xB8, patch->ame_d1r[3], patch_prev->ame_d1r[3], opm_state->cache_invalid);

		opm_tx(i + 0xC0, patch->dt2_d2r[0], patch_prev->dt2_d2r[0], opm_state->cache_invalid);
		opm_tx(i + 0xC8, patch->dt2_d2r[1], patch_prev->dt2_d2r[1], opm_state->cache_invalid);
		opm_tx(i + 0xD0, patch->dt2_d2r[2], patch_prev->dt2_d2r[2], opm_state->cache_invalid);
		opm_tx(i + 0xD8, patch->dt2_d2r[3], patch_prev->dt2_d2r[3], opm_state->cache_invalid);

		opm_tx(i + 0xE0, patch->d1l_rr[0], patch_prev->d1l_rr[0], opm_state->cache_invalid);
		opm_tx(i + 0xE0, patch->d1l_rr[0], patch_prev->d1l_rr[0], opm_state->cache_invalid);
		opm_tx(i + 0xF0, patch->d1l_rr[0], patch_prev->d1l_rr[0], opm_state->cache_invalid);
		opm_tx(i + 0xF0, patch->d1l_rr[0], patch_prev->d1l_rr[0], opm_state->cache_invalid);

		opm_state->patch_prev = opm_state->patch;
		opm_state->reg_28_cache_prev = opm_state->reg_28_cache;
		opm_state->reg_30_cache_prev = opm_state->reg_30_cache;

		opm_state->cache_invalid = 0;
	}
}

static inline void cut_all_opm_sound(void)
{
	// Silence any lingering channel noise.
	for (uint16_t i = 0; i < 8; i++)
	{
		x68k_opm_set_key_on(i, 0);
		x68k_opm_set_tl(i, 0, 0x7F);
		x68k_opm_set_tl(i, 1, 0x7F);
		x68k_opm_set_tl(i, 2, 0x7F);
		x68k_opm_set_tl(i, 3, 0x7F);
	}
}

void xt_start_playing(Xt *xt, int16_t frame, uint16_t repeat)
{
	for (uint16_t i = 0; i < ARRAYSIZE(xt->chan); i++)
	{
		if (xt->chan[i].type != XT_CHANNEL_OPM) continue;
		xt->chan[i].opm.cache_invalid = 1;
	}
	cut_all_opm_sound();
	xt->repeat_frame = repeat;
	xt->playing = 1;
	xt->current_ticks_per_row = xt->track.ticks_per_row;
	xt->tick_counter = 0;

	if (frame >= 0) xt->current_frame = frame;
	if (xt->current_frame >= xt->track.num_frames)
	{
		xt->current_frame = xt->track.num_frames - 1;
	}
	xt->current_phrase_row = 0;
}

void xt_stop_playing(Xt *xt)
{
	cut_all_opm_sound();
	xt->playing = 0;
}
