#include "xt_irq.h"
#include "irq.h"

volatile uint16_t g_xt_vbl_pending;
//volatile uint16_t g_xt_opm_pending;

static void *s_old_vbl_isr;

void xt_irq_init(void)
{
	
	s_old_vbl_isr = _iocs_b_intvcs(0x46, g_irq_vbl);

	// Enable the VBL int.
	volatile uint8_t *ierb = (volatile uint8_t *)0xE88009;
	volatile uint8_t *imrb = (volatile uint8_t *)0xE88015;
	*ierb = 0x40;
	*imrb = 0x40;

	// Set OPM int.
//	_iocs_opmintst(g_irq_opm);

	// TODO: How are you supposed to use this?
	// _iocs_vdispst(g_irq_vbl, 0, 0);
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
	volatile uint8_t *ierb = (volatile uint8_t *)0xE88009;
	volatile uint8_t *imrb = (volatile uint8_t *)0xE88015;
	*ierb &= ~0x40;
	*imrb &= ~0x40;

	_iocs_b_intvcs(0x46, s_old_vbl_isr);
}
