# Novium OS — Roadmap & To-Do

An honest look at where the project stands and what's on the list. Roughly ordered by dependency — the stuff at the top unlocks everything below it.

---

## Done

- [x] Project layout and directory structure
- [x] Basic kernel types
- [x] Boot chain: `boot.S → setup.S → bootstrap.S → hw_init.c → kernel_main`
- [x] Protected mode switch
- [x] VGA text-mode console ("Hello World" is up on screen)
- [x] Low-level PS/2 port I/O helpers
- [x] Linker script

---

## Next up: interrupt handling

This is the big milestone. Right now interrupts are stubs, so the CPU can't react to anything. The keyboard driver, PIT timer, and scheduler all depend on this.

- [ ] Set up the IDT (interrupt descriptor table)
- [ ] ISR stubs for CPU exceptions (div-by-zero, page fault, etc.)
- [ ] Remap the PIC so hardware IRQs don't collide with exceptions
- [ ] IRQ dispatch — route IRQs to the right C handler
- [ ] PIT timer driver (~100 Hz tick, needed for scheduling later)
- [ ] PS/2 keyboard driver — read scancodes via IRQ1

Once this lands, the kernel can actually respond to input instead of sitting in a `while(1)` loop.

---

## Roadmap (after interrupts are solid)

- [ ] VBE / framebuffer graphics mode
- [ ] Memory management — paging + page allocator
- [ ] Basic preemptive scheduling
- [ ] Simple filesystem (VFS + initrd)
- [ ] Boot from hard drive instead of floppy
- [ ] Basic IPC between kernel threads/processes

---

## Long-term

- [ ] Lightweight desktop interface
- [ ] Userspace (once the kernel is more complete)

---

## Notes

- `mm/`, `ipc/`, `kernel/`, `lib/`, `fs/`, `userspace/` and parts of `arch/` are still stubs — they'll get filled in as the kernel progresses.
- `hw_init.c` already calls `idt_init()`, `pic_init()`, `timer_init(100)`, and `keyboard_init()`. Once we implement those functions, they'll get picked up automatically — no wiring changes needed.