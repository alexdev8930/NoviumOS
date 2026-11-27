#ifndef NOVIUM_BOOT_INFO_H
#define NOVIUM_BOOT_INFO_H

#include "types.h"

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MULTIBOOT_INFO_MEMORY_MAP  (1u << 6)
#define MULTIBOOT_INFO_MODS        (1u << 3)
#define MULTIBOOT_INFO_FRAMEBUFFER (1u << 12)

typedef enum {
	MultibootMemoryAvailable = 1,
	MultibootMemoryReserved = 2,
	MultibootMemoryAcpiReclaimable = 3,
	MultibootMemoryAcpiNvs = 4,
	MultibootMemoryBadRam = 5
} MultibootMemoryType;

struct multiboot_info {
	u32 flags;
	u32 mem_lower;
	u32 mem_upper;
	u32 boot_device;
	u32 cmdline;
	u32 modules_count;
	u32 modules_addr;
	u32 syms[4];
	u32 mmap_length;
	u32 mmap_addr;
	u32 drives_length;
	u32 drives_addr;
	u32 config_table;
	u32 boot_loader_name;
	u32 apm_table;
	u32 vbe_control_info;
	u32 vbe_mode_info;
	u16 vbe_mode;
	u16 vbe_interface_seg;
	u16 vbe_interface_off;
	u16 vbe_interface_len;
	u64 framebuffer_addr;
	u32 framebuffer_pitch;
	u32 framebuffer_width;
	u32 framebuffer_height;
	u8  framebuffer_bpp;
	u8  framebuffer_type;
} __attribute__((packed));

_Static_assert(__builtin_offsetof(struct multiboot_info, framebuffer_addr) == 88,
               "multiboot_info framebuffer_addr offset mismatch");

struct multiboot_mmap {
	u32 size;
	u64 base_addr;
	u64 length;
	u32 type;
};

struct multiboot_module {
	u32 mod_start;
	u32 mod_end;
	u32 string;
	u32 reserved;
};

struct boot_info {
	u32  multiboot_magic;
	u32  multiboot_info_addr;
	u32  memory_map_addr;
	u32  memory_map_length;
	u64  framebuffer_addr;
	u32  framebuffer_pitch;
	u32  framebuffer_width;
	u32  framebuffer_height;
	u8   framebuffer_bpp;
	u8   framebuffer_type;
	bool framebuffer_available;
};

#endif
