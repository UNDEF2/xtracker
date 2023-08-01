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

// Get the number of OPM timer ticks that have fired since the last call.
uint16_t xt_get_opm_ticks(void);

// TODO: Register OPM callback

#endif  // XT_IRQ_H
