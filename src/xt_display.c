#include "xt_display.h"
#include "x68000/x68k_vbl.h"
#include <iocs.h>

void apply_mode(const XtDisplayMode *mode)
{
	x68k_crtc_init(&mode->crtc);
	x68k_vidcon_init(&mode->vidcon);

	x68k_vbl_wait_for_vblank();
	x68k_pcg_init(&mode->pcg);
}

// Initialize with a list of display modes. Mode 0 is applied to the video
// chipset.
void xt_display_init(XtDisplay *d, const XtDisplayMode **modes,
                     int16_t num_modes)
{
	d->modes = modes;
	d->num_modes = num_modes;
	d->current_mode = 0;

	apply_mode(d->modes[0]);
}

const XtDisplayMode *xt_display_get_mode(const XtDisplay *d)
{
	return d->modes[d->current_mode];
}

// Go to the next display mode.
void xt_display_cycle_mode(XtDisplay *d)
{
	d->current_mode++;
	if (d->current_mode >= d->num_modes)
	{
		d->current_mode = 0;
	}
	apply_mode(d->modes[d->current_mode]);
}

void xt_display_on_key(XtDisplay *d, XtKeyEvent e)
{
	if (e.name == XT_KEY_HELP)
	{
		xt_display_cycle_mode(d);
	}
}
