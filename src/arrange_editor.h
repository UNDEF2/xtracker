#ifndef ARRANGE_EDITOR_H
#define ARRANGE_EDITOR_H

#include <stdint.h>

#include "xt_track.h"
#include "xt_keys.h"

#include "ui/arrange_render.h"

typedef struct XtArrangeEditor
{
	int16_t frame;
	int16_t column;

	int16_t entry_cursor;  // -1 is inactive; 0 is high digit, 1 is lowest.

	XtArrangeRenderer *r;
} XtArrangeEditor;

void xt_arrange_editor_init(XtArrangeEditor *a, XtArrangeRenderer *r);

void xt_arrange_editor_on_key(XtArrangeEditor *a, XtTrack *t, XtKeyEvent e);

void xt_arrange_editor_set_fnlabels(void);


#endif  // ARRANGE_EDITOR_H