#ifndef NOVIUM_IRQ_H
#define NOVIUM_IRQ_H

#include <novium/types.h>


// all CPU registers pushed onto the stack during an interrupt
struct registers {
    u32 gs, fs, es, ds;         
    u32 edi, esi, ebp, esp;    
    u32 ebx, edx, ecx, eax;
    u32 int_no;             
    u32 err_code;      
    u32 eip, cs, eflags;      
    u32 user_esp, ss;         
} __attribute__((packed));

typedef void (*irq_handler_t)(struct registers *regs);

#define IRQ_BASE    0x20      
#define IRQ_COUNT   16
#define INT_VECTORS 256 

void idt_init(void);
void pic_init(void);
void irq_register(int irq, irq_handler_t handler);
void irq_unregister(int irq);
void irq_enable(void); // sti 
void irq_disable(void); // cli

#endif