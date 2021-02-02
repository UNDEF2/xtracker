#ifndef _XT_RENDER_H
#define _XT_RENDER_H

#include "xt.h"

#include "xt_track.h"

#define XT_RENDER_CELL_PIXELS 8
#define XT_RENDER_CELL_WIDTH_TILES 8
#define XT_RENDER_VISIBLE_WIDTH_TILES 56
#define XT_RENDER_NT_WIDTH_TILES 64

typedef struct XtChannelRenderState
{
	// ID of the last phrase that was drawn. Used to diff with the current
	// phrase, and determine if a repaint is needed.
	int16_t last_phrase_id;
	// Mark a channel as in need of a repaint. This can be from a change of
	// the channel phrase number, or a user input (e.g. editing a column).
	int8_t dirty;
	// Whether or not this channel is visible on the name table.
	// As the nametable is only 64 cells wide, channel 8 and channel 0 occupy
	// the same target spot on the nametable. As the editor scrolls to the
	// right, channel 0 goes "off-camera", at which point channel 8 is now
	// visible.
	int8_t active;
} XtChannelRenderState;

typedef struct XtTrackRenderer
{
	XtChannelRenderState channel[XT_TOTAL_CHANNEL_COUNT];

	// "Camera" position.
	int16_t cam_x, cam_y;

	// Shadow copies of the row highlight option from Xt.XtConfig, updated in
	// xt_track_renderer_tick(). When a new value is taken, NT1 is redrawn
	// accordingly.
	int16_t row_highlight[2];
} XtTrackRenderer;

void xt_track_renderer_init(XtTrackRenderer *r);

void xt_track_renderer_repaint_channel(XtTrackRenderer *r, uint16_t channel);

void xt_track_renderer_tick(XtTrackRenderer *r, Xt *xt, uint16_t frame);

void xt_track_renderer_set_camera(XtTrackRenderer *r, int16_t x, int16_t y);

#endif
