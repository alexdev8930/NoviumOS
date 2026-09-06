# Memory Map

Addresses are physical 32-bit addresses. Paging is currently disabled, so
physical and virtual addresses are identical.

## Fixed Hardware Regions

- **0x00000000–0x000003FF** — real-mode IVT
- **0x00000400–0x000004FF** — BIOS Data Area
- **0x000B8000–0x000B8FFF** — VGA text memory
  (`0xB8000`, 80x25 cells, 4000 bytes used)
- **0x000A0000–0x000FFFFF** — reserved VGA, option ROM, BIOS, and low-memory area

## GRUB Kernel Layout

- **0x00100000** — kernel start and Multiboot header
- **0x00100000–0x00100FFF** — Multiboot header/loadable metadata area
- **0x00101000–0x00103FFF** — kernel text
- **0x00104000–0x0010B6BF** — kernel BSS
- **0x00104000–0x00107FFF** — bootstrap stack inside BSS
- **0x0010C000** — aligned kernel end

The kernel is loaded by GRUB from its ELF image at `0x00100000`.
The exact kernel end changes as the kernel grows.

## Boot Information

GRUB passes:

- **EAX** — Multiboot 1 magic value: `0x2BADB002`
- **EBX** — physical address of the Multiboot information structure

The Multiboot information address and memory-map address are dynamic and must
not be hardcoded. PMM should read the usable-memory regions from GRUB's memory
map.

## Protected-Mode Segments

- **CS = 0x10** — kernel code segment
- **DS = ES = FS = GS = SS = 0x18** — kernel data and stack segment

## Current Status

- No paging yet
- No PMM yet
- No VMM yet
- No heap
- Kernel end is currently `0x0010C000`
- Free memory must be determined from the GRUB Multiboot memory map