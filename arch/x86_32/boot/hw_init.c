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

	struct boot_info boot = {0};
	boot.multiboot_magic = multiboot_magic;
	boot.multiboot_info_addr = multiboot_info_addr;

	/* Multiboot 2: skip 8-byte info header, then walk 8-byte aligned tags */
	if (multiboot_magic == MULTIBOOT2_BOOTLOADER_MAGIC && multiboot_info_addr != 0) {
		struct multiboot2_tag *tag = (struct multiboot2_tag *)(multiboot_info_addr + 8);

		while (tag->type != MULTIBOOT2_TAG_TYPE_END) {
			u32 tag_size = tag->size;
			if (tag_size < 8) {
				break; /* malformed tag */
			}

			switch (tag->type) {
			case MULTIBOOT2_TAG_TYPE_MEMORY_MAP: {
				struct multiboot2_tag_mmap *mmap_tag = (struct multiboot2_tag_mmap *)tag;
				boot.memory_map_addr = (u32)mmap_tag->entries;
				boot.memory_map_length = tag_size - sizeof(struct multiboot2_tag_mmap);
				break;
			}

			case MULTIBOOT2_TAG_TYPE_FRAMEBUFFER: {
				struct multiboot2_tag_framebuffer *fb = (struct multiboot2_tag_framebuffer *)tag;
				if (fb->framebuffer_type == 1) { /* linear RGB */
					boot.framebuffer_addr = fb->framebuffer_addr;
					boot.framebuffer_pitch = fb->framebuffer_pitch;
					boot.framebuffer_width = fb->framebuffer_width;
					boot.framebuffer_height = fb->framebuffer_height;
					boot.framebuffer_bpp = fb->framebuffer_bpp;
					boot.framebuffer_type = fb->framebuffer_type;
					boot.framebuffer_available = true;
				}
				break;
			}

			case MULTIBOOT2_TAG_TYPE_MODULE: {
				struct multiboot2_tag_module *mod = (struct multiboot2_tag_module *)tag;
				boot.module_addr = mod->mod_start;
				boot.module_length = mod->mod_end - mod->mod_start;
				break;
			}
			}

			/* tags are 8-byte aligned */
			tag = (struct multiboot2_tag *)(((u32)tag + tag_size + 7) & ~7u);
		}
	}

	kernel_main(&boot);
}
