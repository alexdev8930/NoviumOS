#include <kernel/irq.h>
#include <drivers/novium/input.h>
#include <kernel/timer.h>

void kernel_main(void);


void hw_init(void) {
    irq_disable();      // cli while wiring IDT/PIC 

    idt_init();
    pic_init();

    timer_init(100);
    keyboard_init();

    irq_enable();

    kernel_main();
}