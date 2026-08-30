#include <novium/cpu.h>
#include <asm/cpu.h>

void cpu_idle(void) {
    cpu_hlt();
}
