#include "ui/section_title.h"
#include "ui/metrics.h"
#include "util/cgprint.h"
#include "palette.h"

void ui_section_title_draw(const char *s, int16_t x, int16_t y, int16_t w, int16_t h)
{
	// Back black rectangle area.
	cgbox(XT_UI_PLANE, XT_PAL_BACK, x, y, w, h);
	// Title.
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT1, s,
	        x + XT_UI_TITLEBAR_TEXT_OFFS_X,
	        y + XT_UI_TITLEBAR_TEXT_OFFS_Y);
	// Accent line (vert)
	cgbox(XT_UI_PLANE, XT_PAL_ACCENT2,
	      x, y,
	      1, XT_UI_TITLEBAR_LINE_H);
	// Accent line (horiz)
	cgbox(XT_UI_PLANE, XT_PAL_ACCENT2,
	      x, y + XT_UI_TITLEBAR_LINE_H,
	      w, 1);
}
