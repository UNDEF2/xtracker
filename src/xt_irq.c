#include "x68000/x68k_irq.h"
#include "xt_irq.h"
#include "irq.h"

volatile uint16_t g_xt_vbl_pending;
//volatile uint16_t g_xt_opm_pending;

static void *s_old_vbl_isr;

static uint8_t s_old_ierb;
static uint8_t s_old_imrb;

static uint8_t s_old_ipl;

void xt_irq_init(void)
{
	// temporarily disable IRQs while we poke IRQ configuration
	s_old_ipl = set_ipl(IPL_ALLOW_NONE);

	s_old_vbl_isr = _iocs_b_intvcs(0x46, g_irq_vbl);

	// Enable the VBL int.
	volatile uint8_t *ierb = (volatile uint8_t *)0xE88009;
	volatile uint8_t *imrb = (volatile uint8_t *)0xE88015;
	s_old_ierb = *ierb;
	s_old_imrb = *imrb;
	*ierb = 0x40;
	*imrb = 0x40;

	// Set OPM int.
	//	_iocs_opmintst(g_irq_opm);

	// TODO: How are you supposed to use this?
	// _iocs_vdispst(g_irq_vbl, 0, 0);

	set_ipl(IPL_ALLOW_ALL);
}

void xt_irq_wait_vbl(void)
{
	g_xt_vbl_pending = 1;
	while (g_xt_vbl_pending) {}
}

//void xt_irq_wait_opm(void)
//{
//	g_xt_opm_pending = 1;
//	while (g_xt_opm_pending) {}
//}

void xt_irq_shutdown(void)
{
	set_ipl(IPL_ALLOW_NONE);
	volatile uint8_t *ierb = (volatile uint8_t *)0xE88009;
	volatile uint8_t *imrb = (volatile uint8_t *)0xE88015;
	*ierb = s_old_ierb;
	*imrb = s_old_imrb;

	_iocs_b_intvcs(0x46, s_old_vbl_isr);
	set_ipl(s_old_ipl);
}
