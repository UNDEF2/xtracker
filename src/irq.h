#ifndef IRQ_H
#define IRQ_H

volatile uint16_t vbl_ticks;

void irq_vbl(void);  // <-- src/irq.a68

#endif  // IRQ_H
