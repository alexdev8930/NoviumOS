#include <kernel/irq.h>
#include <novium/types.h>


void kernel_main(void);

/* --- FREESTANDING ARCHITECTURE PLACEHOLDERS --- */
__attribute__((weak)) void timer_init(int hz) { (void)hz; }
__attribute__((weak)) void keyboard_init(void) {}

/* low-level assembly bindings to control CPU interrupt flags */
static inline void disable_interrupts(void) {
    __asm__ __volatile__("cli");
}

static inline void enable_interrupts(void) {
    __asm__ __volatile__("sti");
}

void hw_init(void) {

    disable_interrupts();
  
    idt_init();       
    
    pic_init();       

    timer_init(100);  
    keyboard_init();  

    kernel_main(); 
    
}
