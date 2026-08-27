#include <drivers/novium/console.h>
#include <novium/debug.h>
#include <drivers/novium/input.h>
#include <novium/boot_info.h>

 
void kernel_main(struct boot_info *boot) {
    console_clear();
    console_write("NoviumOS v0.1.0\n");

    if (boot != NULL && boot->magic == BOOT_INFO_MAGIC) {
        console_write("Boot info valid. Boot drive: ");
        debug_print_dec(boot->boot_drive_id);
        console_write("\n");
    } else {
        console_write("WARNING: no valid boot info\n");
    }

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
