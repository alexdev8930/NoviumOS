void kernel_main(void);

/* --- TEMPORARY PLACEHOLDERS --- 
 * We mark them as '__attribute__((weak))' so that later on, when you write 
 * the real versions in your drivers, they will automatically overwrite these!
 */
__attribute__((weak)) void gdt_init(void) {}
__attribute__((weak)) void idt_init(void) {}
__attribute__((weak)) void pic_init(void) {}
__attribute__((weak)) void timer_init(int hz) { (void)hz; } // (void)hz prevents an 'unused variable' warning
__attribute__((weak)) void keyboard_init(void) {}


void hw_init(void) {
    gdt_init();       
    idt_init();       
    pic_init();       
    timer_init(100);  
    keyboard_init();  

    kernel_main(); 
}
