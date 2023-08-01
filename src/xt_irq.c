#include "xt_irq.h"
#include "xbase/mfp.h"
#include <stdbool.h>

volatile bool s_vbl_pending;
volatile uint16_t s_opm_ticks;

static void *s_old_opm_isr;
static void *s_old_vbl_isr;


static void vbl_handler(void) __attribute__((interrupt_handler));
static void vbl_handler(void)
{
	s_vbl_pending = false;
}

static void opm_handler(void) __attribute__((interrupt_handler));
static void opm_handler(void)
{
	s_opm_ticks++;
}

void xt_irq_init(void)
{
	s_opm_ticks = 0;
	s_vbl_pending = false;
	s_old_vbl_isr = xb_mfp_set_interrupt(XB_MFP_INT_VDISP, vbl_handler);
	s_old_opm_isr = xb_mfp_set_interrupt(XB_MFP_INT_FM_SOUND_SOURCE, opm_handler);
}

void xt_irq_wait_vbl(void)
{
	s_vbl_pending = true;
	while (s_vbl_pending) {}
}

void xt_irq_shutdown(void)
{
	xb_mfp_set_interrupt(XB_MFP_INT_VDISP, s_old_vbl_isr);
	xb_mfp_set_interrupt(XB_MFP_INT_FM_SOUND_SOURCE, s_old_opm_isr);
}

// Get the number of OPM timer ticks that have fired since the last call.
uint16_t xt_get_opm_ticks(void)
{
	const uint16_t ret = s_opm_ticks;
	s_opm_ticks = 0;
	return ret;
}
