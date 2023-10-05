#ifndef XT_MOD_H
#define XT_MOD_H

#include <stdint.h>

typedef enum XtModWaveType
{
	MOD_TRIANGLE,
	MOD_SINE,
	MOD_SQUARE,
} XtModWaveType;

// Modulator struct, used for vibrato and tremolo.
typedef struct XtMod
{
	XtModWaveType wave_type;
	uint8_t index;  // Index into wave table.
	uint8_t accumulator;  // Adds to index when it rolls over (15 - speed).

	uint8_t intensity;
	uint8_t speed;  // Accumulator rate. 0 is a magic value to disable the mod.
	
	// The output; this is the tonal offset to be applied to the note.
	int16_t value;
} XtMod;

void xt_mod_tick(volatile XtMod *mod);

#endif  // XT_MOD_H
