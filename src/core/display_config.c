#include "core/display_config.h"

#include "common.h"
#include "xbase/pcg.h"
#include "xbase/crtc.h"
#include "xbase/util/display.h"
#include "xbase/util/crtcgen.h"

#include <dos.h>
#include <iocs.h>
#include <string.h>

//
// Display Modes
//

// Base config for high x low res scan, and 2 x 256-color CG planes.
#define CRT_FLAGS_BASE 0x0101
#define VIDCON_PRIO 0x31E4
#define VIDCON_FLAGS 0x007F

static const XBCrtcGenParam s_crtc_params[] =
{
	// 31k
	{
		/*htotal*/0x5B,
		/*hsync*/0x08,
		/*hstart*/0x11,
		/*hsize*/512,
		/*vtotal*/2*0x11B,
		/*vsync*/2*0x02,
		/*vstart*/2*0x14,
		/*vsize*/256*2,
		/*ext_h_adj*/0x2C,
		/*crtc_flags*/CRT_FLAGS_BASE | 0x10,
		/*pcg_mode=*/0x0010,
		/*prio*/VIDCON_PRIO,
		/*vidcon_flags*/VIDCON_FLAGS,
	},
	// 15k
	{
		/*htotal*/0x4D,
		/*hsync*/0x09,
		/*hstart*/0x0A,
		/*hsize*/512,
		/*vtotal*/0x11B,
		/*vsync*/0x02,
		/*vstart*/0x14,
		/*vsize*/256,
		/*ext_h_adj*/0x1B,
		/*crtc_flags*/CRT_FLAGS_BASE,
		/*pcg_mode*/0x0000,
		/*prio*/VIDCON_PRIO,
		/*vidcon_flags*/VIDCON_FLAGS,
	},
};

static XBDisplayMode s_display_modes[ARRAYSIZE(s_crtc_params)];

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

	// Generate display modes.
	for (uint16_t i = 0; i < ARRAYSIZE(s_crtc_params); i++)
	{
		xb_crtcgen_set(&s_crtc_params[i], &s_display_modes[i]);
	}
	xb_display_init(&s_display, s_display_modes, ARRAYSIZE(s_display_modes));

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
