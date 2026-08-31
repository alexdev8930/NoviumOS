# Memory Map

Addresses are physical, 32-bit flat model (no paging yet). All values are `0x` hex.

## Core Memory Allocation
- **0x00000000 (1 KB) — Real-mode IVT:** CPU exceptions and hardware IRQs
  (256 × 4-byte vectors, `0x00000000`–`0x000003FF`).
- **0x00000400 (256 B) — BIOS Data Area (BDA):** hardware/state scratch.
- **0x00000500 (~638 KB) — Conventional RAM:** free/usable by OS loaders
  (`0x00000500`–`0x0009FFFF`). The EBDA sits just below `0x000A0000` (size/location
  read from the BDA word at `0x0000040E`).
- **0x000A0000 (128 KB) — VGA memory:** `0xB8000` = text buffer
  (80×25×2 = 4000 bytes).
- **0x000C0000 (128 KB) — Video/option ROMs:** card firmware.
- **0x000E0000 (128 KB) — System BIOS ROM shadow:** read-only system board BIOS.
- **0x00100000 (1 MB+) — Kernel image:** loaded at 1 MB (`KERNEL_LOAD_ADDR`).
  ~1 MB used; the rest grows upward for page frames and other kernel memory.

## Boot-time layout

Same codebase mapping, but your own style:

- **0x7C00** — boot.S (MBR)
- **0x7E00** — setup.S
- **0x8600** — kernel chunk 1 scratch
- **0xA000** — kernel chunk 2 scratch
- **0xC400** — kernel chunk 3 scratch
- **0x100000** — final kernel load
- **0x90000** — bootstrap stack (setup → kernel)
- **0x5000** — boot drive ID handoff byte
- **0xB8000** — VGA text memory


### Segment registers after setup.S
- CS = 0x08, DS = ES = SS = 0x10 (flat 32-bit data)

## Notes
- No paging. No SMP. No heap.
- Free memory starts just above the kernel image (`0x100000 + kernel_blob_size`),
  and is reserved as 4 KiB-aligned page frames once the page allocator exists.
