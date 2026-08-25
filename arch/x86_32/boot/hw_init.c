#include <asm/irq.h>
#include <drivers/novium/input.h>
#include <novium/timer.h>
#include <novium/boot_info.h>
#include "boot.h"

void kernel_main(struct boot_info *boot);


void hw_init(void) {
    irq_disable();      // cli while wiring IDT/PIC 

    idt_init();
    pic_init();

    timer_init(100);
    keyboard_init();

    irq_enable();

    struct boot_info boot;
    boot.magic = BOOT_INFO_MAGIC;
    boot.boot_drive_id = *(volatile u8 *)BOOT_DRIVE_ADDR;
    boot.memory_size_kb = 0;     
    boot.reserved[0] = 0; boot.reserved[1] = 0; boot.reserved[2] = 0;
    boot.reserved[3] = 0; boot.reserved[4] = 0;

    kernel_main(&boot);
}