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
- [x] Basic scancode keyboard drivers

---

## Next up: Better keyboard driver

Right now it just prints scancodes instead of keys

- [ ] Better keyboard driver
- [ ] PIT timer driver (~100 Hz tick, needed for scheduling later)


Once this lands, the kernel can actually respond to input instead of sitting in a `while(1)` loop.

---

## Roadmap 

- [ ] Add 64 bit support
- [ ] Add hard drive support (maybe)
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

- `mm/`, `ipc/`, `kernel/`, `lib/`, `fs/`, `userspace/` and parts of `arch/` are still stubs — they'll get filled in as the kernel progresses.
