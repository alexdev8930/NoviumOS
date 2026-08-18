# Boot Flow

Novium OS boots in stages, each one setting up just enough for the next to take over. Here's how control flows from the moment the BIOS hands us the CPU to the point where the kernel is running.

## 1. Boot Sector (`arch/x86_32/boot/boot.S`)

The BIOS loads the first 512 bytes of the disk (the boot sector) into memory at `0x7C00` and jumps to it in 16-bit real mode. This tiny assembly stub uses BIOS disk interrupts to read the rest of the bootloader setup files (`setup.S`) and the raw kernel image from disk into conventional memory, then hands control over.

## 2. Setup (`arch/x86_32/boot/setup.S`)

Loaded right after the boot sector at `0x7E00` (truncated to 2 KB). While still in 16-bit real mode, this stage does the heavy lifting of early hardware setup: it parses the kernel's ELF program headers and copies the kernel segments to their true linked destination at **1 MB** (`0x00100000`). Once the memory copying is complete, it enables the A20 line, loads a basic GDT, sets the PE bit in `cr0` to **switch into 32-bit protected mode**, and jumps to the kernel.

## 3. Bootstrap (`arch/x86_32/kernel/bootstrap.S`)

This is the kernel's true entry point (`bootstrap_entry`). It runs in 32-bit protected mode with the kernel loaded at 1 MB. Its job is to get the C environment ready:

- Reloads segment registers with flat data segments (`0x10`)
- Sets up a proper stack (`stack_top` in the `.bss` section)
- Clears the BSS section (zero-initialized data)
- Calls `hw_init()` — the first C function

## 4. Hardware Init (`arch/x86_32/boot/hw_init.c`)

`hw_init()` is where the kernel starts doing real work in C. It runs before `kernel_main()` and sets up the core hardware abstractions:

1. **Disables interrupts** (`cli`) — nothing should fire until we're ready
2. **`idt_init()`** — Sets up the Interrupt Descriptor Table so the CPU knows where to jump when an interrupt or exception occurs
3. **`pic_init()`** — Remaps the Programmable Interrupt Controller so hardware IRQs (keyboard, timer, etc.) don't collide with CPU exceptions
4. **`timer_init(100)`** — Configures the PIT for ~100 Hz ticks (weak stub for now)
5. **`keyboard_init()`** — Initializes the PS/2 keyboard driver (weak stub for now)
6. **`kernel_main()`** — Hands control to the kernel's main entry point

## 5. Kernel Main (`init/main.c`)

`kernel_main()` is the kernel's C entry point. Right now it's minimal — it clears the screen, prints "Hello World" via the VGA text-mode console, and halts. As the kernel grows, this is where higher-level initialization (scheduler, memory manager, drivers) will live.

## Visual Summary


```
BIOS
  │
  ▼
boot.S       (0x7C00)  — 16-bit real mode, load setup + kernel
  │
  ▼
setup.S      (0x7E00)  — switch to 32-bit protected mode, load kernel to 1 MB
  │
  ▼
bootstrap.S  (1 MB)    — set up stack, clear BSS
  │
  ▼
hw_init.c              — IDT, PIC, timer, keyboard
  │
  ▼
main.c                 — kernel_main()
```
