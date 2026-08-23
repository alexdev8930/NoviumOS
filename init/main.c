#include <drivers/novium/console.h>
#include <kernel/debug.h>
#include <drivers/novium/input.h>

 
void kernel_main(void) {
    console_clear();
    console_write("Hello World\n");
    console_write("scancodes: ");

    for (;;) {
        int sc = keyboard_pop();
        if (sc >= 0) {
            debug_print_hex8((u8)sc);
            console_putchar(' ');
        }
    }

    while(1); 
}
