#include "ui/fnlabels.h"
#include "util/cgprint.h"

void ui_fnlabel_set(uint16_t i, const char *s)
{
	if (i >= 10) return;
	static const int16_t box_w = 1 + 6 * 7;
	static const int16_t box_h = 9;
	static const int16_t box_x_margin = 6;
	static const int16_t box_y = 256 - box_h - 1;

	int16_t box_x = box_x_margin + (i * (box_w + box_x_margin)) + ((i >= 5) ? (2 * box_x_margin) : 0);
	cgbox(0, 15, box_x, box_y, box_x + box_w, box_y + box_h);
	cgprint(0, 1, s, box_x + 1, box_y + 1);
}
