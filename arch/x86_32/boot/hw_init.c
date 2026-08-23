#include <kernel/irq.h>

void kernel_main(void);

// weak defaults until the real drivers exist 
__attribute__((weak)) void timer_init(int hz) { (void)hz; }
__attribute__((weak)) void keyboard_init(void) {}

void hw_init(void) {
    irq_disable();      // cli while wiring IDT/PIC 

    idt_init();
    pic_init();

    timer_init(100);
    keyboard_init();


    kernel_main();
}