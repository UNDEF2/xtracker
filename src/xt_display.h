// Monitor config management.
#ifndef XT_DISPLAY_H
#define XT_DISPLAY_H

#include <stdint.h>

#include "x68000/x68k_pcg.h"
#include "x68000/x68k_crtc.h"
#include "x68000/x68k_vidcon.h"
#include "xt_keys.h"

typedef struct XtDisplayMode
{
	X68kCrtcConfig crtc;
	X68kPcgConfig pcg;
	X68kVidconConfig vidcon;
} XtDisplayMode;

typedef struct XtDisplay
{
	const XtDisplayMode **modes;
	int16_t num_modes;

	int16_t current_mode;
} XtDisplay;

// Initialize with a list of display modes. Mode 0 is applied to the video
// chipset.
void xt_display_init(XtDisplay *d, const XtDisplayMode **modes,
                     int16_t num_modes);

// Get the current display mode information.
const XtDisplayMode *xt_display_get_mode(const XtDisplay *d);

// Go to the next display mode.
void xt_display_cycle_mode(XtDisplay *d);

// Do a mode cycle if HELP is pressed.
void xt_display_on_key(XtDisplay *d, XtKeyEvent e);

#endif  // XT_DISPLAY_H
