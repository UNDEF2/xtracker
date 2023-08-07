#include "core/display_config.h"

#include "common.h"
#include "xbase/pcg.h"
#include "xbase/crtc.h"
#include "xbase/util/display.h"

#include <dos.h>
#include <iocs.h>
#include <string.h>

//
// Display Modes
//

// Base config for high x low res scan, and 2 x 256-color CG planes.
#define CRT_FLAGS_BASE 0x0101

// 512 x 256 @ 55Hz
static const XBDisplayMode mode_15k =
#define CRT_FLAGS CRT_FLAGS_BASE
{
	{
		0x004C, 0x0009, 0x000A, 0x004A,
		0x011C, 0x0002, 0x0014, 0x0014,
		0x20, CRT_FLAGS
	},
	{
		0x00FF, 0x000E, 0x0014, 0x0000
	},
	{
		(CRT_FLAGS >> 8), 0x31E4, 0x007F
	}
};
#undef CRT_FLAGS

// 512 x 512 @ 55Hz (line doubled internal 256 lines)
static const XBDisplayMode mode_31k =
#define CRT_FLAGS (CRT_FLAGS_BASE | 0x0010)
{
	{
		0x005B, 0x0008, 0x0011, 0x0051,
		0x0237, 0x0005, 0x0026, 0x0226,
		0x1B, CRT_FLAGS
	},
	{
		0x00FF, 0x0015, 0x0026, 0x0010
	},
	{
		(CRT_FLAGS >> 8), 0x31E4, 0x007F
	}
};
#undef CRT_FLAGS
#undef CRT_FLAGS_BASE

static const XBDisplayMode *display_modes[] =
{
	&mode_15k,
	&mode_31k
};

//
// Display Configuration
//

static XBDisplay s_display;
static int16_t s_crtmod_prev;

void display_config_init(void)
{
	// Initialize video mode.
	s_crtmod_prev = _iocs_crtmod(-1);

	// Disable some Human68k features we do not need
	_iocs_g_clr_on();  // Clear CG planes
	_iocs_b_curoff();  // Turn off cursor
	_iocs_ms_curof();  // Turn off mouse cursor
	_iocs_skey_mod(0, 0, 0);  // Don't show key legend

	// Let Human know we are using all planes to prevent calculator, etc.
	_iocs_tgusemd(0, 2);  // CG planes
	_iocs_tgusemd(1, 2);  // TX planes

	_iocs_b_clr_al();

	xb_display_init(&s_display, display_modes, ARRAYSIZE(display_modes));

	// Clear PCG memory
	xb_pcg_set_disp_en(false);
	memset((void *)XB_PCG_TILE_DATA, 0, 0x4000);
	memset((void *)XB_PCG_SPR_TABLE, 0, 0x400);
	memset((void *)XB_PCG_BG0_NAME, 0, 0x2000);
	memset((void *)XB_PCG_BG1_NAME, 0, 0x2000);
	xb_pcg_set_bg0_txsel(0);
	xb_pcg_set_bg1_txsel(1);
	xb_pcg_set_bg0_enable(true);
	xb_pcg_set_bg1_enable(true);
	xb_pcg_set_bg0_xscroll(0);
	xb_pcg_set_bg1_xscroll(0);
	xb_pcg_set_bg0_yscroll(0);
	xb_pcg_set_bg1_yscroll(0);
	xb_pcg_set_disp_en(true);

	// Set default scroll values
	xb_crtc_set_text_xscroll(0);
	xb_crtc_set_text_yscroll(0);
	xb_crtc_set_gp0_xscroll(0);
	xb_crtc_set_gp0_yscroll(0);
	xb_crtc_set_gp1_xscroll(0);
	xb_crtc_set_gp1_yscroll(0);
	xb_crtc_set_gp2_xscroll(0);
	xb_crtc_set_gp2_yscroll(0);
	xb_crtc_set_gp3_xscroll(0);
	xb_crtc_set_gp3_yscroll(0);
}

void display_config_cycle_modes(void)
{
	xb_display_cycle_mode(&s_display);
}

void display_config_shutdown(void)
{
	_iocs_g_clr_on();
	_iocs_skey_mod(-1, 0, 0);
	_iocs_tgusemd(0, 0);
	_iocs_tgusemd(1, 0);

	_iocs_crtmod(s_crtmod_prev);  // Restore original video mode
}
