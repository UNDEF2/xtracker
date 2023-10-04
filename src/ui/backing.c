#include "ui/backing.h"
#include "ui/metrics.h"
#include "util/cgprint.h"
#include "palette.h"

#define XT_UI_LOGO_TILE 0x80

void ui_backing_draw(void)
{
	// Backing (black)
	cgbox(XT_UI_PLANE, XT_PAL_BACK,
	      XT_UI_AREA_X, XT_UI_AREA_Y,
	      XT_UI_AREA_W, XT_UI_AREA_H);

	// Logo
	cgtile(XT_UI_PLANE, XT_UI_LOGO_X, XT_UI_LOGO_Y,
	       XT_UI_LOGO_TILE, 16, 3);
}
