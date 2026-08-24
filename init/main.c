#include <drivers/novium/console.h>
#include <kernel/debug.h>
#include <drivers/novium/input.h>

 
void kernel_main(void) {
    console_clear();
    console_write("NoviumOS v0.0.2\n");
    console_write("System ready. You can now type inside the console:\n");
    console_write("> ");

    for (;;) {
        int key = keyboard_getchar();
        
        if (key < 0) {
            continue;
        }

        console_putchar((char)key);

        if (key == '\n') {
            console_write("> ");
        }
    }    
}
