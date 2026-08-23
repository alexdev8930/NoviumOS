#include <kernel/irq.h>
#include <drivers/novium/input.h>

void kernel_main(void);

// weak defaults until the real drivers exist 
__attribute__((weak)) void timer_init(int hz) { (void)hz; }

void hw_init(void) {
    irq_disable();      // cli while wiring IDT/PIC 

    idt_init();
    pic_init();

    timer_init(100);
    keyboard_init();

    __asm__ __volatile__("sti");

    kernel_main();
}