#include "util/transpose.h"

XtNote xt_transpose_note(XtNote note, int16_t semitones)
{
	if (note == XT_NOTE_NONE) return note;
	if (note == XT_NOTE_OFF) return note;
	if (note == XT_NOTE_CUT) return note;
	int16_t octave = ((note & XT_NOTE_OCTAVE_MASK) >> 4) + semitones / 12;
	int16_t tone = (note & XT_NOTE_TONE_MASK);

	if (semitones > 0)
	{
		semitones = semitones % 12;
		tone += semitones;
		if (tone > XT_NOTE_B)
		{
			if (octave < 7)
			{
				tone -= XT_NOTE_B;
				octave++;
			}
			else
			{
				tone = XT_NOTE_B;
			}
		}
	}
	else if (semitones < 0)
	{
		semitones = -semitones;
		semitones = semitones % 12;

		tone -= semitones;
		if (tone < 0)
		{
			if (octave > 0)
			{
				tone += XT_NOTE_B;
				switch (tone)
				{
					default:
						break;
					case XT_NOTE_NG_D:
					case XT_NOTE_NG_E:
					case XT_NOTE_NG_F:
						tone = XT_NOTE_B;
						break;
				}
				octave--;
			}
			else
			{
				tone = XT_NOTE_C;
			}
		}
	}

	if (octave >= 8) octave = 7;
	else if (octave < 0) octave = 0;

	return (octave << 4) | tone;
}
