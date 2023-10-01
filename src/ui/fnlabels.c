#include "ui/fnlabels.h"
#include "ui/metrics.h"
#include "util/cgprint.h"
#include "xt_palette.h"

void ui_fnlabel_set(uint16_t i, const char *s)
{
	if (i >= 10) return;
	static const int16_t box_w = 1 + 6 * 7;
	static const int16_t box_h = 9;
	static const int16_t box_x_margin = 6;
	static const int16_t box_y = 256 - box_h - 1;

	const int16_t box_x = box_x_margin + (i * (box_w + box_x_margin)) + ((i >= 5) ? (2 * box_x_margin) : 0);
	cgbox(XT_UI_PLANE, XT_PAL_MAIN, box_x, box_y, box_w, box_h);
	cgprint(XT_UI_PLANE, XT_PAL_BACK, s, box_x + 1, box_y + 1);
}
