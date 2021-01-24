#include "xt_vbl.h"
#include "irq.h"

volatile uint16_t g_xt_vbl_pending;

static void *s_old_vbl_isr;

void xt_vbl_init(void)
{
	s_old_vbl_isr = _iocs_b_intvcs(0x46, irq_vbl);

	// Enable the VBL int.
	volatile uint8_t *ierb = (volatile uint8_t *)0xE88009;
	volatile uint8_t *imrb = (volatile uint8_t *)0xE88015;
	*ierb = 0x40;
	*imrb = 0x40;
}

void xt_vbl_wait(uint16_t n)
{
	g_xt_vbl_pending = 1;
	do
	{
		while (g_xt_vbl_pending) {}
	}
	while (n > 0);
}

void xt_vbl_shutdown(void)
{
	volatile uint8_t *ierb = (volatile uint8_t *)0xE88009;
	volatile uint8_t *imrb = (volatile uint8_t *)0xE88015;
	*ierb &= ~0x40;
	*imrb &= ~0x40;

	_iocs_b_intvcs(0x46, s_old_vbl_isr);
}
