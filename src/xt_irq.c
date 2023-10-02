#include "xt_irq.h"
#include "xbase/mfp.h"
#include "xbase/opm.h"
#include <stdbool.h>

volatile bool s_vbl_pending;

static void *s_old_opm_isr;
static void *s_old_vbl_isr;

static void (*s_opm_irq_handler)(void);

static void vbl_handler(void) __attribute__((interrupt_handler));
static void vbl_handler(void)
{
	s_vbl_pending = false;
}

static void opm_handler(void) __attribute__((interrupt_handler));
static void opm_handler(void)
{
	if (s_opm_irq_handler) s_opm_irq_handler();
}

void xt_irq_init(void)
{
	s_vbl_pending = false;
	xt_irq_set_opm(NULL, -1);
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
	xb_opm_set_timer_flags(OPM_TIMER_FLAG_F_RESET_A | OPM_TIMER_FLAG_F_RESET_B);
	xb_mfp_set_interrupt(XB_MFP_INT_VDISP, s_old_vbl_isr);
	xb_mfp_set_interrupt(XB_MFP_INT_FM_SOUND_SOURCE, s_old_opm_isr);
}

void xt_irq_set_opm(void (*handler)(void), int16_t period)
{
	xb_opm_set_timer_flags(OPM_TIMER_FLAG_F_RESET_A);
	s_opm_irq_handler = NULL;

	if (period < 0 || handler == NULL) return;

	s_opm_irq_handler = handler;
	xb_opm_set_clka_period(period & 0x2FF);
	xb_opm_set_timer_flags(OPM_TIMER_FLAG_IRQ_EN_A | OPM_TIMER_FLAG_LOAD_A | OPM_TIMER_FLAG_F_RESET_A);
}

void xt_irq_set_opm_irq_handler(void (*p)(void))
{
	s_opm_irq_handler = p;
}
