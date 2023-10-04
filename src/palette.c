#include "palette.h"

#include "common.h"
#include <stdint.h>
#include "xbase/vidcon.h"

// PCG and text plane palette.
static const uint16_t kpcg_palette[] =
{
	// Line 1 - Normal rows
	XB_PAL_RGB8(0x00, 0x00, 0x00),
	XB_PAL_RGB8(0x50, 0x50, 0x50),  // Separator
	XB_PAL_RGB8(0x30, 0x30, 0x30),  // Empty data grey
	XB_PAL_RGB8(0xB0, 0xB0, 0xB0),  // Note column
	XB_PAL_RGB8(0x30, 0xB0, 0x60),  // Instrument column
	XB_PAL_RGB8(0x60, 0x10, 0xB0),  // Effects command
	XB_PAL_RGB8(0x20, 0x88, 0xB0),  // Effects param
	0, // Unused
	XB_PAL_RGB8(0x20, 0x20, 0x40),  // Highlight backing
	XB_PAL_RGB8(0xD8, 0x00, 0xF8),  // Highlight main
	XB_PAL_RGB8(0x5A, 0x00, 0xA8),  // Highlight sub
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused

	// Line 2 - / 16 highlight
	XB_PAL_RGB8(0x00, 0x00, 0x00),
	XB_PAL_RGB8(0x50, 0x50, 0x50),  // Separator
	XB_PAL_RGB8(0x40, 0x40, 0x40),  // Empty data grey
	XB_PAL_RGB8(0xD0, 0xD0, 0xD0),  // Note column
	XB_PAL_RGB8(0x38, 0xD0, 0x70),  // Instrument column
	XB_PAL_RGB8(0x70, 0x20, 0xD0),  // Effects command
	XB_PAL_RGB8(0x28, 0x98, 0xD0),  // Effects param
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused

	// Line 3 - / 4 highlight
	XB_PAL_RGB8(0x00, 0x00, 0x00),
	XB_PAL_RGB8(0x50, 0x50, 0x50),  // Separator
	XB_PAL_RGB8(0x50, 0x50, 0x50),  // Empty data grey
	XB_PAL_RGB8(0xF8, 0xF8, 0xF8),  // Note column
	XB_PAL_RGB8(0x50, 0xF8, 0x70),  // Instrument column
	XB_PAL_RGB8(0x80, 0x30, 0xF8),  // Effects command
	XB_PAL_RGB8(0x40, 0xA8, 0xF8),  // Effects param
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
};

// Graphic Plane palette (256 color)
static const uint16_t kgp_palette[256] =
{
	[XT_PAL_TRANSPARENT] = 0,
	[XT_PAL_BACK] = XB_PAL_RGB8(0x00, 0x00, 0x00) | 0x0001,
	[XT_PAL_MAIN] = XB_PAL_RGB8(0xFF, 0xFF, 0xFF),
	[XT_PAL_ACCENT1] = XB_PAL_RGB8(0x00, 0xFF, 0xFF),
	[XT_PAL_ACCENT2] = XB_PAL_RGB8(0xFF, 0x00, 0x70),
	[XT_PAL_INACTIVE] = XB_PAL_RGB8(0x2F, 0x2F, 0x3F),
};

void xt_palette_init(void)
{
	// PCG palette.
	for (uint16_t i = 0; i < ARRAYSIZE(kpcg_palette); i++)
	{
		xb_vidcon_set_pcg_color(0x10 + i, kpcg_palette[i]);
	}

	// CG / GVRAM palette.
	for (uint16_t i = 0; i < ARRAYSIZE(kgp_palette); i++)
	{
		xb_vidcon_set_gp_color(i, kgp_palette[i]);
	}
	// TODO: This
}
