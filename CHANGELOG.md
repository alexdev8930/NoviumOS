# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com)

**Versioning works like this:** We just keep counting up 0.1.0 -> 0.2.0 ect, 0.9.0 -> 1.0.0 ect i can also do this 1.5.0 -> 2.0.0 for major updates.

## [Unreleased]

## 0.4.1 - 2026-09-10

### Fixed
- Validate physical page bounds before freeing

## 0.4.0 - 2026-09-09

### Added
- Added basic bitmap page allocator

### Changed
- Restyled Makefile and added CPPFLAGS and QEMU_FLAGS

## 0.3.0 - 2026-09-05

### Added
- Added `memory_map.md`
- Standard GRUB Multiboot 1 boot support
- Automated GRUB ISO builds and a clean `make run` target for QEMU

### Changed
- Scrapped the old custom BIOS bootloader for a GRUB-loaded kernel ELF
- Swapped my custom boot metadata for GRUB's multiboot info structure

### Fixed
- Saved GRUB's EAX/EBX register parameters before they could get clobbered
- Fixed segment selectors to play nice with GRUB's flat GDT layout
- Used a linker `KEEP` directive to retain the Multiboot header in the loadable image

## 0.2.0 - 2026-08-30

### Changed
- Made `cpu.h` all arches
- Changed `sched.c` to have actual cpu scheduling instead of just logic

### Added
- Demo tasks in `main.c`
- Added a low-level context switcher, `switch.S`
- Added an all arch cpu_idle function

## 0.1.6 - 2026-08-28

### Fixed
- Fixed some scheduler task switching bugs
- Fixed task blocking issues
- Fixed time slice handling
- Fixed dead task queue handling

### Added
- C-based scheduler subsystem with task lifecycle management
- Task states for ready, running, blocked, and dead tasks
- Round-robin and priority scheduling policies
- Task creation, lookup, blocking, waking, and termination
- Task priorities, time slices, runtime tracking, and switch counters
- Scheduler statistics and task counting
- Scheduler ready, blocked, and dead queues
- Idle task for the scheduler
- Scheduler locking and unlocking primitives

### Changed
- `sched.c` is now included in the kernel build
- Scheduler tracks runtime and scheduling ticks

## 0.1.5 - 2026-08-28

### Added
- `printf added in stdio.c` and basic functions in `string.c`
- `kernel_main` now declared in `init.h`
- `stdio.h` and `string.h` are now finished

### Changed
- `kernel_main` takes `struct boot_info *` (bootloader→kernel handoff start)

## 0.1.0 - 2026-08-26

### Fixed
- PIT: `timer_get_ticks`, no 64-bit division, div-by-zero guard

### Added
- x86_32 BIOS boot chain, boot sector, protected-mode entry, and flat kernel layout
- VGA text-mode console output in QEMU
- Working QWERTY keyboard drivers
- PIT timer driver with ~100 Hz tick rate
- ISR stubs for CPU core exceptions (divide-by-zero, page faults, etc.)
- Full interrupt infrastructure: IDT setup, PIC remapping, and IRQ dispatch routing


