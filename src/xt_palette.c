#include "xt_palette.h"

#include "common.h"
#include <stdint.h>
#include "x68000/x68k_vidcon.h"

static const uint16_t pcg_palette[] =
{
	// Line 1 - Normal rows
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x50, 0x50, 0x50),  // Separator
	PAL_RGB8(0x30, 0x30, 0x30),  // Empty data grey
	PAL_RGB8(0xB0, 0xB0, 0xB0),  // Note column
	PAL_RGB8(0x30, 0xB0, 0x60),  // Instrument column
	PAL_RGB8(0x60, 0x10, 0xB0),  // Effects command
	PAL_RGB8(0x20, 0x88, 0xB0),  // Effects param
	0, // Unused
	PAL_RGB8(0x20, 0x20, 0x40),  // Highlight backing
	PAL_RGB8(0xD8, 0x00, 0xF8),  // Highlight main
	PAL_RGB8(0x5A, 0x00, 0xA8),  // Highlight sub
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused
	0, // Unused

	// Line 2 - / 16 highlight
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x50, 0x50, 0x50),  // Separator
	PAL_RGB8(0x40, 0x40, 0x40),  // Empty data grey
	PAL_RGB8(0xD0, 0xD0, 0xD0),  // Note column
	PAL_RGB8(0x38, 0xD0, 0x70),  // Instrument column
	PAL_RGB8(0x70, 0x20, 0xD0),  // Effects command
	PAL_RGB8(0x28, 0x98, 0xD0),  // Effects param
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
	PAL_RGB8(0x00, 0x00, 0x00),
	PAL_RGB8(0x50, 0x50, 0x50),  // Separator
	PAL_RGB8(0x50, 0x50, 0x50),  // Empty data grey
	PAL_RGB8(0xF8, 0xF8, 0xF8),  // Note column
	PAL_RGB8(0x50, 0xF8, 0x70),  // Instrument column
	PAL_RGB8(0x80, 0x30, 0xF8),  // Effects command
	PAL_RGB8(0x40, 0xA8, 0xF8),  // Effects param
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

void xt_palette_init(void)
{
	// PCG palette.
	for (int i = 0; i < ARRAYSIZE(pcg_palette); i++)
	{
		x68k_vidcon_set_pcg_color(0x10 + i, pcg_palette[i]);
	}

	// CG / GVRAM palette.
	// TODO: This
}
