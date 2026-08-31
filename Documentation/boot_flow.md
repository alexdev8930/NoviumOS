# Boot Flow

This file explains how the custom bootloader I made for NoviumOS works (soon its gonna have 64 bit support).

## 1. Boot sector - `arch/x86_32/boot/boot.S`

The BIOS loads the first 512 bytes of the disk image to `0x7C00` and jumps to us, still in plain 16-bit real mode. The MBR has 2 jobs:

- **Pull the rest of the boot chain off disk** with BIOS disk reads (`int 0x13`). The setup stage goes to `0x7E00`, then the kernel lands in three fixed chunks at scratch buffers `0x8600`, `0xA000`, and `0xC400`. The kernel image is a raw `objcopy`-flattened binary, so these chunk reads are dumb and predictable - and they match the sizes hardcoded in `arch/x86_32/boot/boot.h`.
- Print a tiny `L` on screen as a sign of life, then jump to setup.

Every address and sector count on this path lives in `boot.h`. It's the single source of truth for the whole boot layout - change a number there and every stage follows.

## 2. Setup - `arch/x86_32/boot/setup.S`

This is the real-mode heavyweight, loaded at `0x7E00`. First thing it does is grab the boot drive out of `%dl` (the one fact the BIOS gave us that we can't get back later) and stash it at `0x5000` for the kernel. Then it prints an `S` and gets to work:

1. **Open the A20 gate** so we can actually use memory above 1 MB - asks the BIOS first, and falls back to poking I/O port `0x92` if the BIOS call fails.
2. **Load a flat GDT and slide into 32-bit protected mode**, jumping to a `code32` section.
3. **Once in 32-bit mode**: reload the flat segment registers, point the stack at `0x90000`, and copy the kernel from its three low-memory scratch buffers up to its permanent home at **1 MB** (`0x100000`). Each chunk copy is exactly `chunk_sectors x 128` dwords - again, straight from `boot.h`. A final `jmp 0x100000` hands control to the kernel.

Worth saying: setup doesn't parse ELF or care about kernel internals. The kernel is a raw binary with known sizes, so the copy is a few fixed `rep movsl` loops - not a loader that walks program headers.

## 3. Bootstrap - `arch/x86_32/kernel/bootstrap.S`

This is the kernel's real entry point (`bootstrap_entry`), already sitting at 1 MB in 32-bit protected mode. Its only job is to make C safe to run:

- Reload the flat data segments into `ds`, `es`, `fs`, `gs`, `ss`.
- Point the stack at the top of its own BSS-resident stack.
- Clear the whole BSS range so every uninitialized global starts at zero (C expects that).
- `call hw_init()` - the first C function to run.

Once we hit `hw_init`, assembly is done. Everything from here on is C.

## 4. Hardware init - `arch/x86_32/boot/hw_init.c`

hw_init() handles early low-level device configuration. Interrupts are disabled via irq_disable() while the subsystems are brought online:

1. `idt_init()` - install the IDT and exception handlers so the CPU knows where to go on interrupts, faults, and syscalls.
2. `pic_init()` - remap the PIC so hardware IRQs (timer, keyboard) don't collide with CPU exceptions.
3. `timer_init(100)` - PIT ticker at ~100 Hz, the heartbeat for scheduling later.
4. `keyboard_init()` - wire up the PS/2 keyboard.
5. `irq_enable()` - safe now, let the world in.
6. **Take a boot_info snapshot** - read back the boot drive we stashed at `0x5000` in setup, pack it into a `struct boot_info` (see `include/novium/boot_info.h`), and pass it along. This is the handoff: the bootloader gathered a fact the kernel can't discover on its own, and `hw_init` turns it into a clean, typed snapshot.
7. `kernel_main()` - hand the snapshot to the kernel itself.

## 5. Kernel main - `init/main.c`

`kernel_main()` takes that `struct boot_info *`, checks the magic is sane, and prints the boot drive we rescued back in setup (`Boot info valid. Boot drive: N`). Then it clears the screen, prints the version banner and a `> ` prompt, and quietly loops: wait for a key, echo it back. It's a living console of exactly one screen of text - and a blank canvas where the scheduler, memory manager, filesystem, and everything after will slot in as the kernel grows.

