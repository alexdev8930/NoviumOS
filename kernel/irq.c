#include "irq.h"

/* TODO: implement real IDT/PIC setup */
void idt_init(void) {}
void pic_init(void) {}
void irq_register(int irq, void (*handler)(void)) {
    (void)irq;
    (void)handler;
}
