#ifndef XT_IRQ_H
#define XT_IRQ_H

#include <stdint.h>
#include <iocs.h>

extern volatile uint16_t g_xt_vbl_pending;

// Register the XT VBlank IRQ.
void xt_irq_init(void);

// Wait for vblank.
void xt_irq_wait_vbl(void);

// Unregister IRQs.
void xt_irq_shutdown(void);

#endif  // XT_IRQ_H
