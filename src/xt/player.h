#ifndef XT_PLAYER_H
#define XT_PLAYER_H

#include <stdint.h>
#include "common.h"
#include "xbase/opm.h"
#include "xt/track.h"
#include "xt/mod.h"
#include "xt/instrument.h"

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
	// Voice properties.
	XtOpmPatch patch;
	int16_t patch_no;
	XBOpmPan pan;
	uint16_t voice;  // Hardware Voice / channel on the OPM.
	bool patch_fresh;  // Patch was just set newly and needs to be fully set.

	// Key state.
	XtOpmKeyCommand key_command;
	XtOpmKeyState key_state;
	XtOpmKeyState key_state_prev;
	int16_t key_on_delay_count;  // Decrements when nonzero on tick.
	int16_t mute_delay_count;
	int16_t cut_delay_count;

	// Pitch data.
	XtNote note;  // Note captured from cell data.
	uint16_t current_pitch;  // Set at the time a note is played.
	uint16_t target_pitch;  // This is what is sent to the register.
	int16_t slide_speed;
	// Pitch patch information calculated after processing pitch.
	uint8_t reg_kc_data;
	uint8_t reg_kf_data;
	int8_t tune;  // Fine offset to be applied to pitch fraction.

	int16_t amplitude;

	// Modulation.
	XtMod mod_vibrato;
	XtMod mod_tremolo;
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

typedef struct XtPlayer
{
	const XtTrack *track;

	// Playback state.
	XtChannelState chan[XT_TOTAL_CHANNEL_COUNT];

	// Position in track
	int16_t current_frame;       // Index into the entire track.
	int16_t current_phrase_row;  // Index into the current phrase.

	// Playback timing
	int16_t current_ticks_per_row;  // Ticks per phrase row.
	int16_t tick_counter;  // Counts down from the period.
	int16_t timer_period;  // Period of timer ticks (OPM).

	bool noise_enable;

	bool repeat_frame;  // Repeats just the current frame.
	bool playing;
} XtPlayer;

void xt_player_init(volatile XtPlayer *xt, const XtTrack *track);
void xt_player_poll(volatile XtPlayer *xt);
void xt_player_update_opm_registers(volatile XtPlayer *xt);

// repeat to cause it to play the same frame repeatedly
void xt_player_start_playing(volatile XtPlayer *xt, int16_t frame, bool repeat_frame);
void xt_player_stop_playing(volatile XtPlayer *xt);
bool xt_player_is_playing(const volatile XtPlayer *xt);
void xt_player_get_playback_pos(const volatile XtPlayer *xt, volatile int16_t *frame, volatile int16_t *row);

#endif  // _XT_H
