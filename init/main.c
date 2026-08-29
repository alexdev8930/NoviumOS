#include <drivers/console.h>
#include <novium/debug.h>
#include <drivers/input.h>
#include <novium/boot_info.h>
#include <novium/init.h>
#include <novium/stdio.h>
 
void kernel_main(struct boot_info *boot) {
    console_clear();
    printf("NoviumOS\n");

    if (boot != NULL && boot->magic == BOOT_INFO_MAGIC) {
        printf("Boot info valid. Boot drive: ");
        printf("Boot info valid. Boot drive: %d\n", boot->boot_drive_id);
        printf("\n");
    } else {
        printf("WARNING: no valid boot info\n");
    }

    printf("System ready. You can now type inside the console:\n");
    console_prompt();
    for (;;) {
        int key = keyboard_getchar();
        
        if (key < 0) {
            continue;
        }

        console_putchar((char)key);

        if (key == '\n') {
            console_prompt();
        }
    }        
}
