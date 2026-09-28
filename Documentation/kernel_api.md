# NoviumOS Kernel API

This file lists the kernel functions that are currently implemented in
NoviumOS. The headers have the final say on exact types and constants.

## Conventions

- The kernel is freestanding and currently targets 32-bit x86.
- `u8`, `u16`, `u32`, `u64`, `s32`, and `size_t` are defined in
  `include/novium/types.h` (the kernel's equivalent to standard headers like
  `<stdint.h>` and `<stddef.h>`).
- Functions that return a pointer or physical address use `0` to report
	failure where noted below.
- Most of these functions are not reentrant. Be careful when calling them from
	interrupt handlers or scheduler code.

## Public Kernel APIs

### Boot and CPU

#### `kernel_main`

```c
void kernel_main(struct boot_info *boot);
```

This is called after the architecture bootstrap code. It initializes the
console, physical memory allocator, paging, scheduler, and first tasks. `boot`
may be `NULL`; invalid or incomplete Multiboot information is reported on the
console.

#### CPU helpers

```c
void cpu_idle(void);
void cpu_disable_irqs(void);
void cpu_enable_irqs(void);
bool cpu_irqs_enabled(void);
static inline void cpu_cli(void);
static inline void cpu_sti(void);
static inline void cpu_hlt(void);
static inline bool cpu_irq_enabled(void);
```

`cpu_idle()` halts until an interrupt arrives. `cpu_disable_irqs()` and
`cpu_enable_irqs()` provide portable interrupt masking across architectures,
while `cpu_irqs_enabled()` checks whether interrupts are currently enabled.
`cpu_cli()` and `cpu_sti()` are architecture-level helpers that disable and
enable maskable interrupts directly. `cpu_hlt()` halts the processor until
the next interrupt and should only be used when interrupt wake-up is possible.
`cpu_irq_enabled()` reports whether maskable interrupts are currently allowed
by reading the interrupt flag in `eflags`.

#### Port I/O

```c
u8  inb(u16 port);
void outb(u16 port, u8 data);
void io_wait(void);
```

These are the low-level x86 port functions. `io_wait()` writes to port `0x80`
and is used between some device-register accesses.

### Console and Debugging

```c
void console_write(const char *str);
void console_putchar(char c);
void console_clear(void);
void console_prompt(void);
void update_hardware_cursor(int position);
extern int user_cmdline_start;
extern bool console_line_echo;

int kprintf(const char *format, ...); /* equivalent to printf() in libc */

void debug_print_hex8(u8 value);
void debug_print_hex32(u32 value);
```

The console uses VGA text memory at `0xB8000` and owns the visible 80x25 page
(`0xB8000`-`0xB8F9F`). `console_putchar()` handles newlines, tabs, and backspace,
and scrolls when a line reaches the last row. Backspace cannot move before
`user_cmdline_start`. `console_prompt()` writes `> ` and records where the
editable command line starts.

All cursor moves go through one clamped setter, so the cursor and the hardware
cursor register stay inside the page and no character is ever written past cell
1999. A newline on the last row scrolls before the next character goes out, and
scrolling takes `user_cmdline_start` up with the prompt, so backspace still
works after a wrap.

`console_line_echo` is set by whoever is echoing a command line. While it is
set, a row that fills up starts the next row with `> `, so a wrapped command
keeps its marker. Printers leave it clear, so ordinary messages wrap on their
own and no `>` shows up in the middle of them.

`kprintf()` supports `%c`, `%s`, `%d`, `%i`, `%u`, `%x`, `%X`, and `%%`. It
always returns `0`. Unsupported format characters are printed literally, and a
format string that ends with a stray `%` stops there instead of running off the
end. The debug helpers print fixed-width uppercase hexadecimal values.

### Interrupts

```c
struct registers { /* register frame supplied by the assembly stubs */ };
typedef void (*irq_handler_t)(struct registers *regs);

void idt_init(void);
void pic_init(void);
void irq_register(int irq, irq_handler_t handler);
void irq_unregister(int irq);
u32  irq_spurious_count(int irq);
void irq_enable(void);
void irq_disable(void);
```

`idt_init()` installs the exception and hardware-interrupt gates. `pic_init()`
remaps the 8259 PIC and masks every line. `irq_register()` accepts IRQ lines
`0` through `15`, installs the handler, and unmasks the line. Invalid lines and
null handlers are ignored. `irq_unregister()` removes the handler and masks
the line again. Handlers run in interrupt context and receive the saved
register frame.

Spurious IRQ7 and IRQ15 are the 8259's usual spurious-interrupt cases and can
be raised even when their lines are masked. The dispatcher reads the in-service
register, switches the command port back to the interrupt request register, and
counts the event. Spurious IRQ7 gets no end-of-interrupt because it has nothing
in service; spurious IRQ15 still acknowledges the master because the slave is
connected through the master's cascade line. The first spurious interrupt on
each line is reported on the console. `irq_spurious_count()` returns the count
for a line, or `0` for invalid lines; a growing count can indicate a missing
acknowledgement, a line dropping before acknowledgement, or PIC initialization
timing.

### Timers

```c
void timer_init(int hz);
u32  timer_get_ticks(void);
u64  timer_uptime_ms(void);
void timer_sleep_ms(u32 milliseconds);
```

`timer_init()` programs PIT channel 0 and registers IRQ 0. Non-positive
frequencies are ignored, the reload divisor is clamped to 16 bits, and the tick
rate stored is the one the PIT actually runs at rather than the requested one.
Before initialization, tick and uptime queries return zero.

`timer_sleep_ms()` rounds the request up to a whole number of ticks, so a
supported duration does not finish early; very large values can overflow the
current 32-bit deadline calculation. For a runnable task, it sets a per-task
wake tick and blocks. The timer interrupt wakes that task when the deadline
passes, so a one-second sleep causes one wakeup rather than one per timer tick.
The idle task and a task with no other runnable task halt and wait in place
instead. If interrupts are disabled, or the timer is not initialized, the call
returns immediately.

### Input

```c
void keyboard_init(void);
int  keyboard_pop(void);
int  keyboard_getchar(void);
```

`keyboard_init()` registers the PS/2 keyboard on IRQ 1. `keyboard_pop()`
returns a raw scancode, or `-1` when the 32-entry ring buffer is empty. The ring
indices are `u16`, allowing the buffer to grow beyond 256 entries without index
truncation. `keyboard_getchar()` uses `hlt()` until it can return a decoded
US-layout character. It handles shift, key release codes, and common control
characters. The `0x3A` Caps Lock scancode currently toggles shift state. Keystrokes
that arrive while the ring buffer is full are discarded.

### Memory and string utilities

```c
void  *memset(void *s, int c, size_t length);
void  *memmove(void *dest, const void *src, size_t length);
void  *memcpy(void *dest, const void *src, size_t length);
size_t strlen(const char *str);
int    strcmp(const char *left, const char *right);

void *kmalloc(size_t size);
void  kfree(void *address);
```

The string functions provide the usual freestanding C behavior. `memmove()`
supports overlapping ranges, and `strcmp()` returns the unsigned-character
difference at the first mismatch. The first heap implementation is in
`mm/heap.c`. The heap stores allocation metadata before returned memory.
`kmalloc()` rounds the requested size plus heap metadata up to a whole number
of pages, allocates a contiguous physical range, and stores the range metadata
before the returned memory. `kfree()` checks the block magic and nonzero page
count before releasing its stored range. Zero-size, overflowing, and
unavailable allocations return `0`. Sub-page block reuse is not supported yet.

## Internal Kernel APIs

### Physical memory

```c
#define PAGE_SIZE 4096u

void PageAllocInit(const struct boot_info *boot);
u32  PageAlloc(void);
u32  PageAllocPages(u32 count);
void PageFree(u32 address);
void PageFreePages(u32 address, u32 count);
u32  PageAllocFreeCount(void);
```

`PageAllocInit()` clears the bitmap, marks valid Multiboot type-1 memory-map
ranges free, and reserves page zero and the linked kernel image. If boot
information has no memory map, initialization leaves the allocator empty.
`PageAlloc()` returns the first free 4 KiB physical page, or `0` if there are no
free pages. `PageAllocPages()` searches for the requested number of consecutive
free pages, marks the complete run as used, and returns the physical address of
its first page. It returns `0` when `count` is zero, exceeds the available
pages, or no contiguous run exists. `PageFree()` frees one page.
`PageFreePages()` frees a contiguous range. Both free functions ignore zero,
unaligned, or out-of-range addresses, and invalid ranges are ignored.
`PageAllocFreeCount()` returns the current number of free pages.

### Paging

```c
#define PAGING_PAGE_SIZE 4096u
#define PAGING_PRESENT   0x001u
#define PAGING_WRITABLE  0x002u
#define PAGING_USER      0x004u

void PagingInit(void);
void PagingMap(u32 virtual_address, u32 physical_address, u32 flags);
void PagingUnmap(u32 virtual_address);
u32  PagingIsEnabled(void);
```

`PagingInit()` builds identity mappings for the full 4 GiB address space and
enables paging. `PagingMap()` and `PagingUnmap()` round addresses down to a
page boundary and invalidate that page in the TLB. `PagingIsEnabled()` is
nonzero after paging has been enabled.

### Scheduler

The scheduler has room for `SchedMaxTasks` (32) task descriptors. New tasks
start with priority `1` and a time slice of `10`. Task names are truncated to
`SchedNameLength - 1` (31) characters.

```c
typedef enum {
    TaskDead = 0, TaskReady, TaskRunning, TaskBlocked
} TaskState;

typedef enum {
		SchedRoundRobin = 0, SchedPriority
} SchedPolicy;

typedef struct Task Task;
typedef struct SchedStats SchedStats;

void SchedInit(void);
Task *SchedCreate(const char *name, u32 eip, u32 esp);
Task *SchedCurrent(void);
Task *SchedFind(u32 id);
u32   SchedTaskCount(void);

void SchedYield(void);
void SchedTick(struct registers *regs);
void SchedPreempt(void);
void SchedBlock(void);
void SchedBlockTask(u32 id);
void SchedUnblock(Task *task);
void SchedWakeTask(u32 id);

void SchedSleepUntil(u32 wake_tick);
u32  SchedWakeExpired(u32 now_tick);
bool SchedHasRunnable(void);

void SchedExit(void);
void SchedKill(u32 id);

void SchedSetPolicy(SchedPolicy policy);
SchedPolicy SchedGetPolicy(void);
void SchedSetPriority(Task *task, u32 priority);
u32  SchedGetPriority(Task *task);
void SchedGetStats(SchedStats *stats);
void SchedLock(void);
void SchedUnlock(void);
void SchedSwitch(u32 *old_esp, u32 new_esp);
```

`SchedCreate()` returns `0` if the task table is full or `name` is null. `eip`
is the task entry address and `esp` is the top of its stack. Task ID `0` is the
idle task, so it cannot be blocked, killed, or exited. `SchedYield()` switches
to the next ready task using the selected policy. `SchedTick()` updates the
runtime counters from the timer interrupt and requests a preemption once a task
exceeds its time slice. The switch itself is not performed there: the timer
handler still runs with its PIC line in service, so switching away would leave
the EOI unsent and the controller would stop delivering interrupts. Instead
`SchedTick()` raises an internal flag, and `SchedPreempt()` performs the switch
from the interrupt epilogue in `isr_dispatch()`, immediately after the EOI.
`SchedLock()` and `SchedUnlock()` use a nesting counter to temporarily stop
scheduling.

Each task has a `WakeTick` field for a blocked sleeper's absolute deadline.
`SchedSleepUntil()` sets the field and blocks the caller, clearing it after the
task wakes; it returns immediately for the idle task or when no other task can
run. `SchedWakeExpired()` is called from the timer interrupt and moves each
blocked sleeper whose deadline has passed to the ready queue, returning the
number it woke. `SchedHasRunnable()` reports whether a task other than the
caller is ready.

`SchedBlockTask()` on the calling task yields after marking it blocked, and
undoes the block if the yield found nothing runnable, so a task is never left
running while marked blocked.

Priority scheduling chooses the highest priority, clamped to `255`. Round
robin scheduling chooses the next ready task by increasing task ID and wraps
back to the start of the ready queue. `SchedGetStats()` needs a non-null output
pointer.

## Planned interfaces

These headers are still placeholders, so they are not usable kernel APIs yet:

- `include/drivers/framebuffer.h`
- `fs/vfs.h` and `fs/initrd.h`
- `ipc/message.h` and `ipc/sync.h`

We will document their contracts here when the implementations land.
