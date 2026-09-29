# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com),
Versioning is similar to [semver](https://semver.org) but is more flexible and similar to how linux does it, and uses custom tags (like `-dev` and `-rc`).

## 0.8-dev - 2026-09-29

### Added
- A permanent `BlockMeta` header on every heap block, holding its magic, size, the size of the block physically before it, and a free flag. The header stays readable whether the block is allocated or on the free list, so a double free is caught instead of corrupting the list.
- A `FreeNode` inside a block, overlapping the payload area, so a free block's list pointers cost nothing and a used block's first 8 bytes are the caller's data.
- Sorted free list helpers in `mm/heap.c`: `HeapListInsert`, `HeapListRemove`, `HeapFindFree` and `HeapSplit`, plus `HeapGrow` and `HeapInit` to seed the heap with 16 pages.
- `HeapInit()` in `<novium/heap.h>`, called once from `kernel_main` after `PageAllocInit()`.
- `HEAP_ALIGN_UP()` and `HEAP_ALIGN` in `mm/heap.c`, so every allocation is returned 8-byte aligned.
- Coalescing in `kfree()`. A freed block merges with free neighbours on both sides, anything absorbed is unlinked from the free list first, and the block after a merge has its `PrevSize` restamped so the next merge finds the right neighbour.
- Page return in `kfree()`. A freed block larger than `HEAP_KEEP_PAGES` pages gives the excess back to the page allocator and stays on the free list as the kept portion, so the heap shrinks after a large allocation instead of holding onto those pages forever. A block touching `HeapEnd` is left alone, since `HeapEnd` has to stay the end of the heap.
- Boot tests covering the reuse, merge, and page return paths, so each is verified at boot rather than assumed.

### Changed
- `kmalloc()` no longer takes whole pages. It searches the free list for a block that fits, splits it when the block is oversized so the remainder stays available, and only falls back to `PageAllocPages()` when the list cannot satisfy the request. A 64 byte request now costs 88 bytes instead of 4096.
- Oversized blocks are cut in two by `HeapSplit()`, and the block after the cut piece has its `PrevSize` restamped so coalescing can still find its neighbours.
- The boot test in `kernel_main()` now checks that a small allocation takes no pages at all, instead of expecting two, since the free list serves it.
- `kfree()` now returns blocks to the free list, and a double free is ignored.

### Known issues
- `HeapGrow()` refuses a growth that is not adjacent to the current heap, so the heap stays one contiguous run and `HeapEnd` stays valid. Lifting that needs a `PageAllocPagesAt()` in the page allocator.
- A freed block that touches `HeapEnd` is never returned to the page allocator, so the tail of the heap is held for the life of the kernel.
- The page return threshold is a compile-time constant rather than something the kernel adapts to its working set.

## 0.7.4-dev - 2026-09-28

### Added
- `bool` support in `<novium/types.h>`, so flags stop pretending to be `int`.
- A packed `VgaColor` palette for the text attribute byte, with a `_Static_assert` on its size, so an enumerator added past 255 fails the build instead of silently widening the byte and shifting every cell after it.
- Enums for what used to be macros or magic numbers: the console geometry (`ConsoleWidth`, `ConsoleHeight`, `ConsoleCells`), the page allocator's `PageState`, the five multiboot memory map types in `MultibootMemoryType`, and `EflagsInterruptEnable`.

### Changed
- Predicates that can only be true or false are `bool` now: `cpu_irqs_enabled()`, `cpu_irq_enabled()`, `SchedHasRunnable()` and `PagingIsEnabled()` return one, and `console_line_echo`, `PagingEnabled` and `PreemptPending` store one.
- The `true` and `false` macros in `<novium/types.h>` are gone in favour of `<stdbool.h>`, so `bool`, `true` and `false` come from the language rather than from `int` literals.
- `SchedLockIrq()` and its callers pass a `bool` instead of a `u32` holding 0 or 1.
- `PageSet()` and `PageSetRange()` take a `PageState` instead of a bare `0` or `1`, so reserving a page reads as `PageStateUsed` at the call site.
- A cleared cell's attribute is `ConsoleAttribute` instead of a literal `0x07`, and `MULTIBOOT_MEMORY_AVAILABLE` is now the shared `MultibootMemoryAvailable` from `<novium/boot_info.h>`.

## 0.7.3-dev - 2026-09-27

### Added
- Portable CPU interrupt control helpers in `<novium/cpu.h>`: `cpu_disable_irqs()`, `cpu_enable_irqs()`, and `cpu_irqs_enabled()`.
- `SchedPreempt()` in `<novium/sched.h>`, which performs a time slice preemption from the interrupt epilogue once the PIC line has been acknowledged.

### Fixed
- `switch.S` not saving and restoring `EFLAGS`, so a task preempted out of an interrupt handler (where the interrupt flag is clear) handed that cleared flag to whichever task it resumed. The resumed task could then reach `hlt` with interrupts masked and never be woken again, which froze the timer and the keyboard. The context now carries `eflags`, and new task stacks are built with `0x202` so a fresh task starts with interrupts enabled.
- A preemption happening inside `SchedTick()`, which ran with the timer's PIC line still in service. Switching tasks there suspended the handler before `isr_dispatch()` sent the EOI, so the controller kept the line in service and stopped delivering `IRQ0` and `IRQ1`. The timer now only requests a preemption, and `SchedPreempt()` runs it after the EOI.
- The context switch no longer re-enables interrupts midway through the switch. It stays atomic and the resumed task's own saved `eflags` are restored, so a task interrupted inside a handler returns with interrupts off and a task that yielded normally returns with them on.
- Protected scheduler queues (`ReadyQueue`, `BlockedQueue`, `DeadQueue`) and task operations against interrupt race conditions using `SchedLockIrq()` and `SchedUnlockIrq()`.
- `SchedWakeExpired()` and `SchedHasRunnable()` are no longer reachable in a state where a stalled handler could delay a sleeper's wakeup.
- Replaced stale descriptor data in `SchedExit()` and `SchedKill()` with `SchedResetTask()`, preventing task information leaks.
- `SchedKill()` no longer looks the target up before checking for a self-kill, which removed a use of `SchedFind()` outside the scheduler lock.

## 0.7.2-dev - 2026-09-26

### Added
- `console_line_echo`, set by the shell while it echoes a command line, so a row that fills up restarts the next one with `> `.

### Fixed
- `console_putchar()` writing past the 80x25 text page on a newline at the last row, which ate the next prompt's `>` and left the cursor off screen.
- The cursor and the hardware cursor register being able to point outside the text page; every move now goes through one clamped `console_set_cursor()`.
- `console_scroll()` not moving `user_cmdline_start`, which broke backspace after the screen wrapped.
- `kprintf()` reading past the end of a format string that ends with `%`.

## 0.7.1-dev - 2026-09-24

### Added
- Blocking `timer_sleep_ms()` on per-task wake ticks, so a sleep costs one wakeup instead of one per timer tick.
- `SchedSleepUntil()`, `SchedWakeExpired()`, and `SchedHasRunnable()` for sleep and wakeup handling.
- Spurious interrupt counters with `irq_spurious_count()`.
- `cpu_irq_enabled()` for checking the interrupt flag.

### Changed
- Spurious IRQ7/15 handling now reads the PIC ISR, restores OCW3 back to the IRR, counts each event, and warns once instead of returning silently.
- Keyboard ring buffer indices widened from `u8` to `u16` so `BUF_SIZE` can grow.
- `timer_init()` clamps the PIT divisor to 16 bits and stores the rate the timer actually runs at.
- Unhandled exceptions now report through `kprintf()`.

### Fixed
- `timer_sleep_ms()` returning immediately for sleeps shorter than one tick, and the truncated tick conversion.
- Sleep tick math no longer needs 64-bit division, so the kernel links without compiler runtime helpers.
- `SchedTick()` no longer reinitialises the scheduler from the timer interrupt when no task is current.
- `SchedYield()` no longer saves a stack pointer through a null task pointer.
- `SchedBlockTask()` undoes the block when a self-block finds nothing else runnable.

## 0.7.0-dev - 2026-09-17

### Added
- Contiguous multi-page physical allocations with `PageAllocPages()`.
- Contiguous physical page range freeing with `PageFreePages()`.
- Multi-page kernel heap allocations and frees through `kmalloc()` and `kfree()`.

## 0.6.0-dev - 2026-09-14

### Added
- Tested allocation/free behavior in the kernel
- Finished the first single-page kernel heap allocator with `kmalloc()` and `kfree()`.
- Added heap allocation metadata validation with a magic value and page tracking.

### Changed
- Finished heap.h header for `kmalloc()` and `kfree()`.

## 0.5.1-dev - 2026-09-11

### Changed
- printf is now `kprintf()`.

## 0.5.0-dev - 2026-09-11

### Added
- Initial x86 32-bit identity-mapped paging.
- Page directory and page table structures covering the full 4 GiB address space.
- `invlpg`-based TLB invalidation for unmapped addresses.
- CR3 loading and CR0 paging bit setup during init.

### Fixed
- Disabled SSE2 generation in the x86_32 kernel build flags.

## 0.4.1-dev - 2026-09-10

### Fixed
- Validate physical page bounds before freeing

## 0.4.0-dev - 2026-09-09

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

note: the repo was private for a bit so thats why it looks like i just made everything in 0.1.0 in one day


