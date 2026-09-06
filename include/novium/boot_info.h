#ifndef NOVIUM_BOOT_INFO_H
#define NOVIUM_BOOT_INFO_H

#include <novium/types.h>

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MULTIBOOT_INFO_MEMORY_MAP  (1u << 6)

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
};

struct multiboot_mmap {
    u32 size;
    u64 base_addr;
    u64 length;
    u32 type;
};

struct boot_info {
    u32 multiboot_magic;
    u32 multiboot_info_addr;
    u32 memory_map_addr;
    u32 memory_map_length;
};

#endif