#include "xt_track.h"

XtPhrase *xt_track_get_phrase(XtTrack *t, uint16_t channel, uint16_t frame)
{
	const uint16_t idx = t->frames[frame].phrase_id[channel];
	return &t->channel_data[channel].phrases[idx];
}
