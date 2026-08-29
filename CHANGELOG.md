# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com), and this
project adheres to [Semantic Versioning](https://semver.org).

## [Unreleased]

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


