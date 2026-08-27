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
- [x] Set up the IDT 
- [x] Remap the PIC so hardware IRQs don't collide with exceptions
- [x] IRQ dispatch — route IRQs to the right C handler
- [x] ISR stubs for CPU exceptions (div-by-zero, page fault, etc.)
- [x] Working QWERTY keyboard drivers
- [x] PIT timer driver (~100 Hz tick, needed for scheduling later)
---

## Next up: Create stdio.h

- [ ] Make stdio.c and the kernel library
- [ ] Add 64 bit support
- [ ] Make scheduling

---

## Roadmap 

- [ ] Make the bootloader boot from hard drive instead of floppy or just switch to GRUB
- [ ] VBE / framebuffer graphics mode
- [ ] Memory management — paging + page allocator
- [ ] Basic preemptive scheduling
- [ ] Simple filesystem (VFS + initrd)
- [ ] Basic IPC between kernel threads/processes

---

## Long-term

- [ ] Lightweight desktop interface
- [ ] Userspace (once the kernel is more complete)

---

## Notes

- `mm/`, `ipc/`, `lib/`, `fs/`, `userspace/` but `arch/` and `kernel/` are mostly done but still have some stubs — they'll get filled in as the kernel progresses.
