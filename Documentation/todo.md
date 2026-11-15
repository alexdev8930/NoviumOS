# Novium OS - Roadmap & To-Do

This is the NoviumOS todo list this lists things that are going to happen in the future and things that are happening right now.

---

For the history of everything completed so far, see [CHANGELOG.md](../CHANGELOG.md).

## Next up (`v0.10.0` – `v1.0.0`)

- [ ] **v0.10.0:** Basic interactive kernel shell (using VFS & `initrd`)
- [ ] **v0.11.0:** Linear framebuffer graphics support & font rendering
- [ ] **v1.0.0:** Simple `ext2` filesystem (clearing remaining `fs/` stubs) & switch bootloader to Limine

---
## Roadmap 

- [ ] Add 64-bit (`x86_64`) support with UEFI instead of 32-bit BIOS
---

## Long-term

- [ ] Lightweight desktop interface
- [ ] Userspace (once the kernel is more complete)

---

## Notes

- `fs/` has stubs for ext2 but it will get filled in as the kernel progresses.
