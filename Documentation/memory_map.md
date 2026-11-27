# Memory Map

Physical addresses are 32-bit addresses and are managed in 4 KiB pages. The
kernel enables identity paging for the complete 32-bit address space, from
`0x00000000` through `0xFFFFFFFF` (4 GiB).

## Fixed Hardware Regions

- **0x00000000–0x000003FF**: real-mode IVT
- **0x00000400–0x000004FF**: BIOS Data Area
- **0x000B8000–0x000B8FFF**: VGA text memory
  (`0xB8000`, 80x25 cells, 4000 bytes used when running `vgacon`)
- **0x000A0000–0x000FFFFF**: reserved VGA, option ROM, BIOS, and low-memory area
- **Linear Framebuffer (dynamic MMIO, e.g. `0xFD000000`)**: 24/32-bpp VBE/GOP
  framebuffer provided by GRUB (`800x600x32` by default for `fbcon`), identity-mapped
  in the upper physical address space

## GRUB Kernel Layout

- **0x00100000**: kernel image load address and `_kernel_start`
- **`.multiboot`**: 8-byte aligned Multiboot 2 header near the start of the image
- **`.text`**: executable code, page-aligned after the Multiboot section
- **`.rodata`**: read-only data, page-aligned
- **`.data`**: initialized writable data, page-aligned
- **`.bss`**: zero-initialized data and stacks
- **`_kernel_end`**: page-aligned end of the kernel image

The kernel is loaded by GRUB from its ELF image at `0x00100000`.
The exact kernel end changes as the kernel grows.

## Boot Information

GRUB passes:

- **EAX**: Multiboot 2 bootloader magic value `0x36D76289`
- **EBX**: physical address of the Multiboot 2 information structure (8-byte
  `total_size` + `reserved` header followed by 8-byte aligned tags)

The Multiboot 2 info structure, `initrd.img` module payload, and memory-map
addresses are dynamic. `hw_init()` walks the Multiboot 2 tag list to extract the
memory map, `initrd` module range, and framebuffer descriptor into `struct boot_info`,
and `PageAllocInit()` reserves the Multiboot 2 info block and `initrd` pages so
they cannot be overwritten by later allocations.

## Protected-Mode Segments

- **CS = 0x10**: kernel code segment
- **DS = ES = FS = GS = SS = 0x18**: kernel data and stack segment

## Current Status

- 4 KiB physical page allocator backed by the GRUB Multiboot 2 memory map
- Automatic page reservation for low memory (`< 1 MiB`), the kernel image, the Multiboot 2 info structure, and the `initrd.img` module payload
- Contiguous multi-page physical allocations and range frees
- Identity paging for the full 4 GiB 32-bit address space
- Runtime page mapping and unmapping helpers
- Kernel heap with a sorted free list, 8-byte aligned sub-page allocations, block splitting, coalescing on free, and returning large free blocks to the page allocator
- Heap growth uses the allocator's `PageAllocPagesAt()` hint-based search, and the heap tracks growth in segment ranges so tail checks are tied to the owning segment rather than one global heap end
- Kernel end changes as the kernel grows
- Free memory is determined from the GRUB Multiboot 2 memory map
