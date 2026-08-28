# Novium OS — Roadmap & To-Do

An honest look at where the project stands and what's on the list. Roughly ordered by dependency — the stuff at the top unlocks everything below it.

---

For the history of everything completed so far, see [CHANGELOG.md](../CHANGELOG.md).


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
