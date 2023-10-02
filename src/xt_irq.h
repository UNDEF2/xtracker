#ifndef XT_IRQ_H
#define XT_IRQ_H

#include <stdint.h>
#include <iocs.h>

// Register the XT IRQ handlers.
void xt_irq_init(void);

// Wait for vblank.
void xt_irq_wait_vbl(void);

// Unregister IRQs.
void xt_irq_shutdown(void);

// Set period to -1 or handler to NULL to disable IRQ generation.
void xt_irq_set_opm(void (*handler)(void), int16_t period);

#endif  // XT_IRQ_H
