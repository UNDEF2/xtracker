#ifndef _XT_H
#define _XT_H

#include <stdint.h>
#include "common.h"
#include "xbase/opm.h"
#include "xt_track.h"
#include "xt_mod.h"
#include "xt_instrument.h"

// Status of a single FM channel's pitch, which we must track to support
// simple portamento effects and vibrato.
typedef struct XtOpmPitch
{
	uint8_t octave;
	XBOpmNote note;
	uint8_t fraction;
} XtOpmPitch;

typedef enum XtOpmKeyState
{
	KEY_STATE_OFF,
	KEY_STATE_ON,
	KEY_STATE_ON_PENDING,
	KEY_STATE_CUT,
} XtOpmKeyState;

typedef enum XtOpmKeyCommand
{
	KEY_COMMAND_NONE,
	KEY_COMMAND_ON,
	KEY_COMMAND_OFF,
} XtOpmKeyCommand;

typedef struct XtOpmChannelState
{
	XtOpmPatch patch;
	XtPan pan_overlay;

	XtMod mod_vibrato;
	XtMod mod_tremolo;

	uint16_t current_pitch;  // Set at the time a note is played.
	uint16_t target_pitch;  // This is what is sent to the register.

	// Pitch patch information calculated after processing pitch.
	uint8_t reg_kc_data;
	uint8_t reg_kf_data;

	int16_t portamento_speed;
	int16_t amplitude;

	XtOpmKeyState key_state;
	XtOpmKeyState key_state_prev;
	int16_t key_on_delay_count;  // Decrements when nonzero on tick.
	int16_t mute_delay_count;
	int16_t cut_delay_count;

	int8_t tune;  // Fine offset to be applied to pitch fraction.

	XtOpmKeyCommand key_command;
} XtOpmChannelState;

typedef struct XtChannelState
{
	XtChannelType type;
	union
	{
		XtOpmChannelState opm;
		// TODO: ADPCM, etc.
	};
} XtChannelState;

// Configuration data that should persist between sessions.
typedef struct XtConfig
{
	int16_t row_highlight[2];
} XtConfig;

typedef struct Xt
{
	XtTrack track;
	// TODO: Channel allocation should be dynamic, and size stored here. This
	// is to allow for flexible configurations.
	// TODO: Channel type should always have parity with the current XtTrack.
	XtChannelState chan[XT_TOTAL_CHANNEL_COUNT];
	XtConfig config;

	uint8_t pitch_table[8 * 12];

	int16_t current_frame;  // Index into the entire track.
	int16_t current_phrase_row;  // Index into the current phrase.

	int16_t current_ticks_per_row;  // Ticks per row.
	int16_t tick_counter;  // Counts down from the period.
	int16_t timer_period;  // Period of timer ticks.

	bool noise_enable;

	bool playing;
	int16_t repeat_frame;
} Xt;

// TODO: Disk format for a track.

void xt_init(Xt *xt);
void xt_poll(Xt *xt);
void xt_update_opm_registers(Xt *xt);

// -1 to resume playback at current frame
// repeat to cause it to play the same frame repeatedly
void xt_start_playing(Xt *xt, int16_t frame, uint16_t repeat);
void xt_stop_playing(Xt *xt);

#endif  // _XT_H
