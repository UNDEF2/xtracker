#ifndef _XT_RENDER_H
#define _XT_RENDER_H

#include "xt.h"

#include "xt_track.h"

typedef struct XtTrackRenderer
{
	// ID of the last phrase that was drawn. Used to diff with the current
	// phrase, and determine if a repaint is needed.
	int16_t last_phrase_num[XT_TOTAL_CHANNEL_COUNT];

	// A channel marked as in need of a repaint. This can be from a mismatch of
	// the channel phrase number, or a user input (e.g. editing a column)
	int8_t channel_dirty[XT_TOTAL_CHANNEL_COUNT];

	int16_t row_highlight[2];
} XtTrackRenderer;

void xt_track_renderer_init(XtTrackRenderer *r);

void xt_track_renderer_repaint_channel(XtTrackRenderer *r, uint16_t channel);

void xt_track_renderer_tick(XtTrackRenderer *r, Xt *xt, uint16_t frame);

#endif
