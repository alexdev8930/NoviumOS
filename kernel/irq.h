#ifndef NOVIUM_IRQ_H
#define NOVIUM_IRQ_H

#include <novium/types.h>

void idt_init(void);
void pic_init(void);
void irq_register(int irq, void (*handler)(void));

#endif