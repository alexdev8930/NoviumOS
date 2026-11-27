#include <asm/cpu.h>
#include <asm/irq.h>
#include <novium/boot_info.h>
#include <novium/drivers/input.h>
#include <novium/init.h>
#include <novium/timer.h>

void hw_init(u32 multiboot_magic, u32 multiboot_info_addr) {
	irq_disable(); /* cli while wiring IDT/PIC */

	idt_init();
	pic_init();

	timer_init(100);
	keyboard_init();

	irq_enable();

	struct boot_info boot;
	boot.multiboot_magic = multiboot_magic;
	boot.multiboot_info_addr = multiboot_info_addr;
	boot.framebuffer_available = false;

	/* verify we actually booted by GRUB and the info pointer exists */
	if (multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC && multiboot_info_addr != 0) {
		struct multiboot_info *mbi = (struct multiboot_info *)multiboot_info_addr;

		/* bit 6 of flags specifies if a valid BIOS memory map array exists */
		if (mbi->flags & MULTIBOOT_INFO_MEMORY_MAP) {
			boot.memory_map_addr = mbi->mmap_addr;
			boot.memory_map_length = mbi->mmap_length;
		} else {
			boot.memory_map_addr = 0;
			boot.memory_map_length = 0;
		}

		/* bit 12 of flags specifies framebuffer info is available */
		if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER) {
			boot.framebuffer_addr = mbi->framebuffer_addr;
			boot.framebuffer_pitch = mbi->framebuffer_pitch;
			boot.framebuffer_width = mbi->framebuffer_width;
			boot.framebuffer_height = mbi->framebuffer_height;
			boot.framebuffer_bpp = mbi->framebuffer_bpp;
			boot.framebuffer_type = mbi->framebuffer_type;
			boot.framebuffer_available = true;
		} else {
			boot.framebuffer_addr = 0;
			boot.framebuffer_pitch = 0;
			boot.framebuffer_width = 0;
			boot.framebuffer_height = 0;
			boot.framebuffer_bpp = 0;
			boot.framebuffer_type = 0;
		}
	} else {
		boot.memory_map_addr = 0;
		boot.memory_map_length = 0;
		boot.framebuffer_addr = 0;
		boot.framebuffer_pitch = 0;
		boot.framebuffer_width = 0;
		boot.framebuffer_height = 0;
		boot.framebuffer_bpp = 0;
		boot.framebuffer_type = 0;
	}

	kernel_main(&boot);
}
