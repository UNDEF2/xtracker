#include "ui/chanlabels.h"
#include "ui/metrics.h"
#include "util/cgprint.h"
#include "palette.h"

#include <stdbool.h>

void ui_chanlabel_set(uint16_t i, uint16_t num, XtChannelType type)
{
	// TODO: Enums / something legible instead of 8 for FM Channel count
	const int16_t draw_x = (i * XT_UI_CHANLABEL_W);
	const char *name = "...";
	switch (type)
	{
		case XT_CHANNEL_OPM:
			name = "OPM";
			break;
		case XT_CHANNEL_ADPCM:
			name = "PCM";
			break;
		default:
			break;

	}
	static const int16_t name_len = 3;
	const int16_t name_offs = ((XT_UI_CHANLABEL_W / 2) + 1) - ((name_len + 1) * 6 / 2);

	cgtile(XT_UI_PLANE, draw_x, XT_UI_CHANLABEL_Y, 0xB0,
	       XT_UI_CHANLABEL_W / 8, XT_UI_CHANLABEL_H / 8);
	cgprint(XT_UI_PLANE, XT_PAL_MAIN, name, draw_x + name_offs, XT_UI_CHANLABEL_Y);

	// Use special hex digits in font
	char num_buffer[2];
	num_buffer[0] = 0x10 + num % 8;
	num_buffer[1] = '\0';
	cgprint(XT_UI_PLANE, XT_PAL_ACCENT2, num_buffer,
	        draw_x + name_offs + (name_len * 6), XT_UI_CHANLABEL_Y);

}
