#ifndef UI_ARRANGE_RENDER_H
#define UI_ARRANGE_RENDER_H

#include "xt_track.h"
#include "ui/track_render.h"

#include <stdbool.h>

/*

Inputs:

For the highlight / scroll center:
* Editor / playback row within track (vertical) --> editor.frame, xt.current_phrase_row
* Editor channel (horizontal) --> editor.column

For overall render:
* Values in arrangement table +/- 3 rows from current row. --> xt.track.frames

*/

typedef struct XtArrangeRenderer
{
	// A collection of rows that represents the phrase IDs currently drawn to
	// the CG plane.
	// These are just a 5-row-high "window" into the whole arrange table, so
	// index 0 just refers to the top-left of the area of focus.
	XtFrame frames[5];
	int16_t frame_id[5];

	struct
	{
		int16_t row, column;
	} edit;

	int16_t last_frame_count;
	bool backing_drawn;
} XtArrangeRenderer;

// Set up the XtArrange struct for its first render and further use.
void xt_arrange_renderer_init(XtArrangeRenderer *a);

// Draw the arrangement table as-needed based on track data and the provided
// navigation position. Pass -1 to the row or column parameters to not update
// positioning.
void xt_arrange_renderer_tick(XtArrangeRenderer *a, const XtTrack *t,
                              int16_t row, int16_t col);

// Mark all frames and the border as needing a redraw.
void xt_arrange_renderer_request_redraw(XtArrangeRenderer *a, bool content_only);

// Mark one column for a redraw.
void xt_arrange_renderer_redraw_col(XtArrangeRenderer *a, int16_t col);

#endif  // UI_ARRANGE_RENDER_H

