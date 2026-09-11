# Novium OS - Roadmap & To-Do

This is the NoviumOS todo list this lists things that are going to happen in the future and things that are happening right now.

---

For the history of everything completed so far, see [CHANGELOG.md](../CHANGELOG.md).


## Next up: Heap allocater (kmalloc/kfree)

- [ ] Heap allocator (kmalloc/kfree) - simple bump allocator or free-list allocator to start
- [ ] Simple filesystem (VFS + initrd maybe ext2)
- [ ] Basic IPC between kernel threads/processes

---

## Roadmap 

- [ ] Add 64 bit support with UEFI instead of BIOS
- [ ] Support framebuffer graphics (VBE for 32-bit BIOS (VBE later maybe) / UEFI GOP for 64-bit)


---

## Long-term

- [ ] Lightweight desktop interface
- [ ] Userspace (once the kernel is more complete)

---

## Notes

- `mm/`, `ipc/`, `lib/`, `fs/`, `userspace/` but `arch/` and `kernel/` are mostly done but still have some stubs — they'll get filled in as the kernel progresses.
