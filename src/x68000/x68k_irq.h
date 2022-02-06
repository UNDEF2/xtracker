#ifndef _X68K_IRQ_H
#define _X68K_IRQ_H

#include <stdint.h>

#define ISR  __attribute__((__interrupt_handler__))

#define IPL_ALLOW_ALL  0
#define IPL_ALLOW_NONE 7

// optimizer should be able to catch cases where return value is unused
static inline uint8_t set_ipl(uint8_t ipl) {
	uint16_t sr;
	__asm volatile ("move.w %%sr, %0" : "=r" (sr));
	uint8_t old_ipl = (sr >> 8) & 7;
	sr &= ~(7 << 8);
	sr |= (uint16_t)ipl << 8;
	__asm volatile ("move.w %0, %%sr" : : "r" (sr));
	return old_ipl;
}

#endif
