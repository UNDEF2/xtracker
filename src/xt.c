#include "xt.h"

#include <stdio.h>
#include <string.h>

#include "x68000/x68k_opm.h"

static inline void xt_read_cell_cmd(Xt *xt, XtFmChannelState *fm_state,
                                    uint8_t cmd, uint8_t arg)
{
	if (cmd == XT_CMD_NONE) return;
	switch (cmd)
	{
		default:
			// TODO: Print an error, probably
			break;
			
		case XT_CMD_TL_OP0:
			fm_state->instrument.reg_60_tl[0] = 0x7F;
			break;
		case XT_CMD_TL_OP1:
			fm_state->instrument.reg_60_tl[1] = 0x7F;
			break;
		case XT_CMD_TL_OP2:
			fm_state->instrument.reg_60_tl[2] = 0x7F;
			break;
		case XT_CMD_TL_OP3:
			fm_state->instrument.reg_60_tl[3] = 0x7F;
			break;

		case XT_CMD_MULT_OP0:
			fm_state->instrument.reg_40_dt1_mul[0] &= 0xF0;
			fm_state->instrument.reg_40_dt1_mul[0] |= arg & 0x0F;
			break;
		case XT_CMD_MULT_OP1:
			fm_state->instrument.reg_40_dt1_mul[1] &= 0xF0;
			fm_state->instrument.reg_40_dt1_mul[1] |= arg & 0x0F;
			break;
		case XT_CMD_MULT_OP2:
			fm_state->instrument.reg_40_dt1_mul[2] &= 0xF0;
			fm_state->instrument.reg_40_dt1_mul[2] |= arg & 0x0F;
			break;
		case XT_CMD_MULT_OP3:
			fm_state->instrument.reg_40_dt1_mul[3] &= 0xF0;
			fm_state->instrument.reg_40_dt1_mul[3] |= arg & 0x0F;
			break;

		case XT_CMD_AMPLITUDE:
			fm_state->amplitude = arg;
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
			if (arg == 0x11) fm_state->reg_20_overlay = 0xC0;
			else if (arg == 0x01) fm_state->reg_20_overlay = 0x80;
			else if (arg == 0x10) fm_state->reg_20_overlay = 0x40;
			else fm_state->reg_20_overlay = 0;
			break;

		case XT_CMD_TUNE:
			fm_state->tune = arg;
			break;

		case XT_CMD_VIBRATO:
			fm_state->mod_vibrato.intensity = arg & 0x0F;
			fm_state->mod_vibrato.speed = (arg >> 4);
			break;
		case XT_CMD_VIBRATO_TYPE:
			fm_state->mod_vibrato.wave_type = arg;
			break;
		case XT_CMD_TREMOLO:
			fm_state->mod_tremolo.intensity = arg & 0x0F;
			fm_state->mod_tremolo.speed = (arg >> 4);
			break;
		case XT_CMD_TREMOLO_TYPE:
			fm_state->mod_vibrato.wave_type = arg;
			break;

		case XT_CMD_SLIDE_UP:
			// TODO: This
		case XT_CMD_SLIDE_DOWN:
			// TODO: This
			break;
		case XT_CMD_MUTE_DELAY:
			fm_state->mute_delay_count = arg;
			break;
		case XT_CMD_NOTE_DELAY:
			fm_state->key_on_delay_count = arg;
			break;
		case XT_CMD_CUT_DELAY:
			fm_state->cut_delay_count = arg;
			break;
	}
}

// Read note and patch data from a cell.
static inline void xt_read_cell_data(const Xt *xt, XtFmChannelState *fm_state,
                                     const XtCell *cell)
{	// Update note, and set it up to be played.
	if (cell->note == XT_NOTE_NONE) return;

	// Update patch information.
	fm_state->instrument = xt->track.instruments[cell->inst];


	
	if (cell->note == XT_NOTE_OFF)
	{
		fm_state->key_state = KEY_STATE_OFF;
		fm_state->key_command = KEY_COMMAND_OFF;
		fm_state->key_on_delay_count = 0;
	}
	else if (cell->note == XT_NOTE_CUT)
	{
		fm_state->key_state = KEY_STATE_CUT;
		fm_state->key_command = KEY_COMMAND_OFF;
		fm_state->key_on_delay_count = 0;
	}
	else
	{
		fm_state->target_pitch.octave = (cell->note & XT_NOTE_OCTAVE_MASK) >> 4;
		fm_state->target_pitch.note = (cell->note & XT_NOTE_TONE_MASK) - 1;
		fm_state->target_pitch.fraction = fm_state->tune;
		fm_state->key_state = KEY_STATE_ON_PENDING;
		fm_state->key_on_delay_count = 0;
	}
}

static inline void xt_update_fm_key_state(XtFmChannelState *fm_state)
{
	if (fm_state->key_on_delay_count > 0)
	{
		fm_state->key_on_delay_count--;
	}

	if (fm_state->key_on_delay_count == 0 &&
	    fm_state->key_state == KEY_STATE_ON_PENDING)
	{
		fm_state->key_on_delay_count = 0;
		fm_state->key_state = KEY_STATE_ON;
		fm_state->key_command = KEY_COMMAND_ON;
	}

	if (fm_state->mute_delay_count > 0)
	{
		fm_state->mute_delay_count--;
		if (fm_state->mute_delay_count == 0)
		{
			fm_state->key_state = KEY_STATE_OFF;
			fm_state->key_command = KEY_COMMAND_OFF;

		}
	}

	if (fm_state->cut_delay_count > 0)
	{
		fm_state->cut_delay_count--;
		if (fm_state->cut_delay_count == 0)
		{
			fm_state->key_state = KEY_STATE_CUT;
			fm_state->key_command = KEY_COMMAND_OFF;
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
}

void xt_tick(Xt *xt)
{
	if (!xt->playing) return;
	// Process all channels for playback.
	for (uint16_t i = 0; i < XT_FM_CHANNEL_COUNT; i++)
	{
		XtFmChannelState *fm_state = &xt->fm_state[i];

		// The bank of phrases associated with this channel.
		const uint16_t idx_base = (XT_PHRASES_PER_CHANNEL * i);
		// The phrase number referred to by the arrangement table.
		const uint16_t idx = xt->track.frames[xt->current_frame].row_idx[i];
		// The phrase as referred to.
		const XtPhrase *phrase = &xt->track.phrases[idx_base + idx];

		// The current cell.
		const XtCell *cell = &phrase->cells[xt->current_phrase_row];

		if (xt->tick_counter == 0)
		{
			xt_read_cell_data(xt, fm_state, cell);
			for (uint16_t i = 0; i < ARRAYSIZE(cell->cmd); i++)
			{
				xt_read_cell_cmd(xt, fm_state, cell->cmd[i].cmd, cell->cmd[i].arg);
			}
		}

		xt_mod_tick(&fm_state->mod_vibrato);
		xt_mod_tick(&fm_state->mod_tremolo);

		if (fm_state->portamento_speed == 0)
		{
			fm_state->current_pitch = fm_state->target_pitch;
		}
		else
		{
			// TODO: Slide towards target pitch at portamento speed.
		}
		
		fm_state->reg_30_cache = fm_state->current_pitch.fraction;
		fm_state->reg_28_cache = fm_state->current_pitch.note |
		                         (fm_state->current_pitch.octave << 4);

		// TODO: Add vibrato to the pitch register caches (not applied to the
		// pitch data itself)

		xt_update_fm_key_state(fm_state);
	}

	xt_playback_counters(xt);
}

static inline void fm_tx(uint8_t addr, uint8_t new_val, uint8_t old_val, uint8_t force)
{
	if (force || new_val != old_val) x68k_opm_write(addr, new_val);
}

void xt_update_opm_registers(Xt *xt)
{
	if (!xt->playing) return;
	// Commit registers based on new state.
	for (uint16_t i = 0; i < XT_FM_CHANNEL_COUNT; i++)
	{
		XtFmChannelState *fm_state = &xt->fm_state[i];
		XtInstrument *inst = &fm_state->instrument;
		XtInstrument *inst_prev = &fm_state->instrument_prev;

		if (fm_state->key_command == KEY_COMMAND_ON)
		{
			x68k_opm_set_key_on(i, 0x0);
			x68k_opm_set_key_on(i, 0xF);
			fm_state->key_command = KEY_COMMAND_NONE;
		}
		else if (fm_state->key_command == KEY_COMMAND_OFF)
		{
			// TODO: Remove once we have TL cache
			if (fm_state->key_state == KEY_STATE_CUT)
			{
				inst->reg_60_tl[0] = 0x7F;
				inst->reg_60_tl[1] = 0x7F;
				inst->reg_60_tl[2] = 0x7F;
				inst->reg_60_tl[3] = 0x7F;
			}
			x68k_opm_set_key_on(i, 0x0);
			fm_state->key_command = KEY_COMMAND_NONE;
		}

		// TODO: Create TL caches, and use that to apply both note cut, and
		//       the channel amplitude settings, to the TL.

		fm_tx(i + 0x28, fm_state->reg_28_cache, fm_state->reg_28_cache_prev, fm_state->cache_invalid);
		fm_tx(i + 0x30, fm_state->reg_30_cache, fm_state->reg_30_cache_prev, fm_state->cache_invalid);

		fm_tx(i + 0x20, inst->reg_20_pan_fl_con | fm_state->reg_20_overlay,
		                inst_prev->reg_20_pan_fl_con | fm_state->reg_20_overlay, 1);
		fm_tx(i + 0x38, inst->reg_38_pms_ams, inst_prev->reg_38_pms_ams, fm_state->cache_invalid);

		fm_tx(i + 0x40, inst->reg_40_dt1_mul[0], inst_prev->reg_40_dt1_mul[0], fm_state->cache_invalid);
		fm_tx(i + 0x48, inst->reg_40_dt1_mul[1], inst_prev->reg_40_dt1_mul[1], fm_state->cache_invalid);
		fm_tx(i + 0x50, inst->reg_40_dt1_mul[2], inst_prev->reg_40_dt1_mul[2], fm_state->cache_invalid);
		fm_tx(i + 0x58, inst->reg_40_dt1_mul[3], inst_prev->reg_40_dt1_mul[3], fm_state->cache_invalid);

		fm_tx(i + 0x60, inst->reg_60_tl[0], inst_prev->reg_60_tl[0], fm_state->cache_invalid);
		fm_tx(i + 0x68, inst->reg_60_tl[1], inst_prev->reg_60_tl[1], fm_state->cache_invalid);
		fm_tx(i + 0x70, inst->reg_60_tl[2], inst_prev->reg_60_tl[2], fm_state->cache_invalid);
		fm_tx(i + 0x78, inst->reg_60_tl[3], inst_prev->reg_60_tl[3], fm_state->cache_invalid);

		fm_tx(i + 0x80, inst->reg_80_ks_ar[0], inst_prev->reg_80_ks_ar[0], fm_state->cache_invalid);
		fm_tx(i + 0x88, inst->reg_80_ks_ar[1], inst_prev->reg_80_ks_ar[1], fm_state->cache_invalid);
		fm_tx(i + 0x90, inst->reg_80_ks_ar[2], inst_prev->reg_80_ks_ar[2], fm_state->cache_invalid);
		fm_tx(i + 0x98, inst->reg_80_ks_ar[3], inst_prev->reg_80_ks_ar[3], fm_state->cache_invalid);

		fm_tx(i + 0xA0, inst->reg_A0_ame_d1r[0], inst_prev->reg_A0_ame_d1r[0], fm_state->cache_invalid);
		fm_tx(i + 0xA8, inst->reg_A0_ame_d1r[1], inst_prev->reg_A0_ame_d1r[1], fm_state->cache_invalid);
		fm_tx(i + 0xB0, inst->reg_A0_ame_d1r[2], inst_prev->reg_A0_ame_d1r[2], fm_state->cache_invalid);
		fm_tx(i + 0xB8, inst->reg_A0_ame_d1r[3], inst_prev->reg_A0_ame_d1r[3], fm_state->cache_invalid);

		fm_tx(i + 0xC0, inst->reg_C0_dt2_d2r[0], inst_prev->reg_C0_dt2_d2r[0], fm_state->cache_invalid);
		fm_tx(i + 0xC8, inst->reg_C0_dt2_d2r[1], inst_prev->reg_C0_dt2_d2r[1], fm_state->cache_invalid);
		fm_tx(i + 0xD0, inst->reg_C0_dt2_d2r[2], inst_prev->reg_C0_dt2_d2r[2], fm_state->cache_invalid);
		fm_tx(i + 0xD8, inst->reg_C0_dt2_d2r[3], inst_prev->reg_C0_dt2_d2r[3], fm_state->cache_invalid);

		fm_tx(i + 0xE0, inst->reg_E0_d1l_rr[0], inst_prev->reg_E0_d1l_rr[0], fm_state->cache_invalid);
		fm_tx(i + 0xE0, inst->reg_E0_d1l_rr[0], inst_prev->reg_E0_d1l_rr[0], fm_state->cache_invalid);
		fm_tx(i + 0xF0, inst->reg_E0_d1l_rr[0], inst_prev->reg_E0_d1l_rr[0], fm_state->cache_invalid);
		fm_tx(i + 0xF0, inst->reg_E0_d1l_rr[0], inst_prev->reg_E0_d1l_rr[0], fm_state->cache_invalid);

		fm_state->instrument_prev = fm_state->instrument;
		fm_state->reg_28_cache_prev = fm_state->reg_28_cache;
		fm_state->reg_30_cache_prev = fm_state->reg_30_cache;

		fm_state->cache_invalid = 0;
	}
}

static inline void cut_all_opm_sound(void)
{
	// Silence any lingering channel noise.
	for (uint16_t i = 0; i < XT_FM_CHANNEL_COUNT; i++)
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
	for (uint16_t i = 0; i < XT_FM_CHANNEL_COUNT; i++)
	{
		xt->fm_state[i].cache_invalid = 1;
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
