# NoviumOS Kernel API

This file lists the kernel functions that are currently implemented in
NoviumOS. The headers have the final say on exact types and constants.

## Conventions

- The kernel is freestanding and currently targets 32-bit x86.
- `u8`, `u16`, `u32`, `u64`, `s32`, and `size_t` are defined in
	`include/novium/types.h` (the kernel's equivalent to standard headers like `<stdint.h>` and `<stddef.h>`).
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
static inline void cpu_cli(void);
static inline void cpu_sti(void);
static inline void cpu_hlt(void);
```

`cpu_idle()` halts until an interrupt arrives. `cpu_cli()` and `cpu_sti()`
disable and enable maskable interrupts. `cpu_hlt()` halts the processor until
the next interrupt and should only be used when interrupt wake-up is possible.

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

int kprintf(const char *format, ...); /* equivalent to printf() in libc */

void debug_print_hex8(u8 value);
void debug_print_hex32(u32 value);
```

The console uses VGA text memory at `0xB8000`. `console_putchar()` handles
newlines, tabs, and backspace. Backspace cannot move before
`user_cmdline_start`. `console_prompt()` writes `> ` and records where the
editable command line starts.

`kprintf()` supports `%c`, `%s`, `%d`, `%i`, `%u`, `%x`, `%X`, and `%%`. It
always returns `0`. Unsupported format characters are printed literally. The
debug helpers print fixed-width uppercase hexadecimal values.

### Interrupts

```c
struct registers { /* register frame supplied by the assembly stubs */ };
typedef void (*irq_handler_t)(struct registers *regs);

void idt_init(void);
void pic_init(void);
void irq_register(int irq, irq_handler_t handler);
void irq_unregister(int irq);
void irq_enable(void);
void irq_disable(void);
```

`idt_init()` installs the exception and hardware-interrupt gates. `pic_init()`
remaps the 8259 PIC and masks every line. `irq_register()` accepts IRQ lines
`0` through `15`, installs the handler, and unmasks the line. Invalid lines and
null handlers are ignored. `irq_unregister()` removes the handler and masks
the line again. Handlers run in interrupt context and receive the saved
register frame.

### Timers

```c
void timer_init(int hz);
u32  timer_get_ticks(void);
u64  timer_uptime_ms(void);
void timer_sleep_ms(u32 milliseconds);
```

`timer_init()` programs PIT channel 0 and registers IRQ 0. Non-positive
frequencies are ignored. Before initialization, tick and uptime queries return
zero, and `timer_sleep_ms()` returns immediately. Sleep uses `hlt()` in a busy
wait loop, so interrupts need to be enabled.

### Input

```c
void keyboard_init(void);
int  keyboard_pop(void);
int  keyboard_getchar(void);
```

`keyboard_init()` registers the PS/2 keyboard on IRQ 1. `keyboard_pop()`
returns a raw scancode, or `-1` when the 32-entry ring buffer is empty.
`keyboard_getchar()` uses `hlt()` until it can return a decoded US-layout
character. It handles shift, caps-lock state, key release codes, and common
control characters. Keystrokes that arrive while the ring buffer is full are
discarded.

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
difference at the first mismatch. `kmalloc()` and `kfree()` are declared in
the memory API, but `kernel/memory.c` is still a system stub. The heap is not
implemented yet.

## Internal Kernel APIs

### Physical memory

```c
#define PAGE_SIZE 4096u

void PageAllocInit(const struct boot_info *boot);
u32  PageAlloc(void);
void PageFree(u32 address);
u32  PageAllocFreeCount(void);
```

`PageAllocInit()` clears the bitmap, marks Multiboot type-1 memory-map ranges
free, and reserves page zero and the linked kernel image. `PageAlloc()` returns
the first free 4 KiB physical page, or `0` if there are no free pages.
`PageFree()` only accepts nonzero, page-aligned addresses in the 32-bit address
space. Invalid addresses are ignored. `PageAllocFreeCount()` returns the
current number of free pages.

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
void SchedBlock(void);
void SchedBlockTask(u32 id);
void SchedUnblock(Task *task);
void SchedWakeTask(u32 id);
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
runtime counters from the timer interrupt, but does not currently force a
context switch. `SchedLock()` and `SchedUnlock()` use a nesting counter to
temporarily stop scheduling.

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
