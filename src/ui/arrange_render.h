#ifndef UI_ARRANGE_RENDER_H
#define UI_ARRANGE_RENDER_H

#include "xt_track.h"

#include <stdbool.h>

#define XT_ARRANGE_RENDER_X 8
#define XT_ARRANGE_RENDER_Y 24

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
	// These are just a 7-row-high "window" into the whole arrange table, so
	// index 0 just refers to the top-left of the area of focus.
	XtFrame frames[7];

	int16_t row;  // The current row to be selected. Must always be valid.
	int16_t column;  // The current column to highlight.

	int16_t draw_x, draw_y;

	bool border_drawn;
} XtArrangeRenderer;

// Set up the XtArrange struct for its first render and further use.
void xt_arrange_renderer_init(XtArrangeRenderer *a, const XtTrack *t);

// Draw the arrangement table as-needed based on track data and the provided
// navigation position. Pass -1 to the row or column parameters to not update
// positioning.
void xt_arrange_renderer_tick(XtArrangeRenderer *a, const XtTrack *t,
                              int16_t row, int16_t col);

// Mark all frames and the border as needing a redraw.
void xt_arrange_renderer_redraw(XtArrangeRenderer *a);

#endif  // UI_ARRANGE_RENDER_H

