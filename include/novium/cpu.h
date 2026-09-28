#ifndef NOVIUM_CPU_H
#define NOVIUM_CPU_H

#include <novium/types.h>

/* Put CPU in low-power halt until next interrupt */
void cpu_idle(void);

/* Disable CPU interrupts */
void cpu_disable_irqs(void);

/* Enable CPU interrupts */
void cpu_enable_irqs(void);

/* Check if CPU interrupts are currently enabled */
bool cpu_irqs_enabled(void);

#endif 
