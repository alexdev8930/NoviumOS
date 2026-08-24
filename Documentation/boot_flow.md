# Boot Flow

Novium doesn't jump from "BIOS" to "kernel" in one step - it climbs a small ladder of stages, each one setting up just enough for the next rung. Here's the whole trip, from the moment the BIOS puts the CPU in the boot sector to the moment you're typing in the console.

## 1. Boot sector - `arch/x86_32/boot/boot.S`

The BIOS loads the first 512 bytes of the disk image at `0x7C00` and jumps to it, still in plain 16-bit real mode. This little MBR has two jobs:

- **Pull the rest of the boot chain off disk** using BIOS disk interrupts (`int 0x13`): the setup stage goes to `0x7E00`, then the kernel lands in three fixed chunks at scratch buffers `0x8600`, `0xA000`, and `0xC400`. The kernel image is a raw, `objcopy`-flattened binary, so these chunk reads are dumb, predictable, and match the sizes hardcoded in `arch/x86_32/boot/boot.h`.
- Print a tiny `L` on screen as a sign of life, remember which drive we booted from (the BIOS hands it to us in `%dl`), then jump to setup.

Every address and sector count along the way lives in `boot.h` - it's the single source of truth for the whole boot layout. Change a number there and every stage follows.

## 2. Setup - `arch/x86_32/boot/setup.S`

This is the real-mode heavyweight, loaded at `0x7E00`. It prints an `S`, then does three things in order:

1. **Open the A20 gate** so we can actually use memory above 1 MB - asks the BIOS first, and falls back to poking I/O port `0x92` if the BIOS call fails.
2. **Load a flat GDT and slide into 32-bit protected mode**, jumping to a `code32` section.
3. **Once in 32-bit mode**: reload flat segment registers, point the stack at `0x90000`, and copy the kernel from its three low-memory scratch buffers up to its permanent home at **1 MB** (`0x100000`). Each chunk copy is exactly `chunk_sectors x 128` dwords - again, straight from `boot.h`. A final `jmp 0x100000` hands control to the kernel.

Note: setup doesn't parse ELF or understand kernel internals. The kernel is a raw binary with known sizes, so the copy is a few fixed `rep movsl` loops, not a loader that walks program headers.

## 3. Bootstrap - `arch/x86_32/kernel/bootstrap.S`

This is the kernel's true entry point (`bootstrap_entry`), already living at 1 MB in 32-bit protected mode. Its job is to make C safe to run:

- Load the flat data segments into `ds`, `es`, `fs`, `gs`, `ss`.
- Set up the stack pointer to the top of its own BSS-resident stack.
- Clear the whole BSS range so every uninitialized global starts at zero (C expects that).
- `call hw_init()` - the first C function to run.

## 4. Hardware init - `arch/x86_32/boot/hw_init.c`

`hw_init()` is where the kernel starts doing real work. It runs with interrupts disabled first (`irq_disable()`), so nothing fires while things are half-built:

1. `idt_init()` - install the IDT and exception handlers so the CPU knows where to go on interrupts, faults, and syscalls.
2. `pic_init()` - remap the PIC so hardware IRQs (timer, keyboard) don't collide with CPU exceptions.
3. `timer_init(100)` - PIT ticker at ~100 Hz, the heartbeat for scheduling later.
4. `keyboard_init()` - wire up the PS/2 keyboard.
5. `irq_enable()` - safe now, let the world in.
6. `kernel_main()` - hand off to the kernel itself.

## 5. Kernel main - `init/main.c`

`kernel_main()` clears the VGA text screen, prints the version banner and a `> ` prompt, then quietly loops: wait for a key, echo it back. It's a living, breathing console of exactly one screen of text - and a blank canvas where the scheduler, memory manager, filesystem, and everything after will plug in as the kernel grows.
