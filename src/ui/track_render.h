#ifndef _XT_TRACK_RENDER_H
#define _XT_TRACK_RENDER_H

#include "xt.h"

#include "xt_track.h"

#define XT_RENDER_CELL_W_PIXELS 8
#define XT_RENDER_CELL_H_PIXELS 8
#define XT_RENDER_CELL_CHARS (5 + (XT_CMD_COL_COUNT*3))
#define XT_RENDER_NT_CHARS 64

#define XT_RENDER_VISIBLE_CHANNELS (512 / (XT_RENDER_CELL_CHARS * XT_RENDER_CELL_W_PIXELS))

typedef struct XtChannelRenderState
{
	// Pointer to last phrase that was drawn. Used to diff with the current
	// phrase, and determine if a repaint is needed.
	const XtPhrase *last_phrase;
	// Mark a channel as in need of a repaint. This can be from a change of
	// the channel phrase number, or a user input (e.g. editing a column).
	bool dirty;
	// Whether or not this channel is visible on the name table.
	// As the nametable is only 64 cells wide, channel 8 and channel 0 occupy
	// the same target spot on the nametable. As the editor scrolls to the
	// right, channel 0 goes "off-camera", at which point channel 8 is now
	// visible.
	bool active;
} XtChannelRenderState;

typedef struct XtTrackRenderer
{
	XtChannelRenderState chan[XT_TOTAL_CHANNEL_COUNT];

	int16_t visible_channels;

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

#endif  // XT_TRACK_RENDER
