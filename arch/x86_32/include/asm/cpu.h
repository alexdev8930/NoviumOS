#ifndef NOVIUM_X86_32_CPU_H
#define NOVIUM_X86_32_CPU_H

static inline void cpu_cli(void) { __asm__ __volatile__("cli" ::: "memory"); }
static inline void cpu_sti(void) { __asm__ __volatile__("sti" ::: "memory"); }
static inline void cpu_hlt(void) { __asm__ __volatile__("hlt" ::: "memory"); }

#endif 
