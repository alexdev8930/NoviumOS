#ifndef NOVIUM_X86_32_CPU_H
#define NOVIUM_X86_32_CPU_H

#include <novium/types.h>

static inline void cpu_cli(void) { __asm__ __volatile__("cli" ::: "memory"); }
static inline void cpu_sti(void) { __asm__ __volatile__("sti" ::: "memory"); }
static inline void cpu_hlt(void) { __asm__ __volatile__("hlt" ::: "memory"); }

/* EFLAGS.IF, bit 9: set while interrupts are enabled. */
enum { EflagsInterruptEnable = 0x200 };

/* True when interrupts are enabled (EFLAGS.IF). */
static inline bool cpu_irq_enabled(void) {
    u32 flags;

    __asm__ __volatile__("pushfl; popl %0" : "=r"(flags) : : "memory");
    return (flags & (u32)EflagsInterruptEnable) != 0;
}

#endif 
