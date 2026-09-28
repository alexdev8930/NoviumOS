#include <novium/cpu.h>
#include <asm/cpu.h>

void cpu_idle(void) {
    cpu_hlt();
}

void cpu_disable_irqs(void) {
    cpu_cli();
}

void cpu_enable_irqs(void) {
    cpu_sti();
}

bool cpu_irqs_enabled(void) {
    return cpu_irq_enabled();
}
