#include <drivers/console.h>
#include <novium/debug.h>
#include <drivers/input.h>
#include <novium/boot_info.h>
#include <novium/init.h>
#include <novium/stdio.h>
#include <novium/sched.h>
#include <novium/timer.h>
#include <novium/cpu.h>
#include <mm/page_alloc.h>
#include <mm/paging.h>

#define TASK_STACK_SIZE 4096
static u8 ShellStack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static u8 WorkerStack[TASK_STACK_SIZE] __attribute__((aligned(16)));

void shell_task(void) {
    kprintf("System ready. You can now type inside the console:\n");
    console_prompt();

    for (;;) {
        int key = keyboard_getchar();
        
        if (key < 0) {
            SchedYield();
            continue;
        }

        console_putchar((char)key);

        if (key == '\n') {
            console_prompt();
        }
    }        
}

void worker_task(void) {
    for (;;) {
        SchedYield();
    }
}

void kernel_main(struct boot_info *boot) {
    console_clear();
    kprintf("NoviumOS\n\n");

    if (boot != NULL && boot->multiboot_magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        kprintf("OK: Multiboot info valid.\n");
        if (boot->memory_map_length != 0) {
            kprintf("OK: Memory map available.\n\n");
        } else {
            kprintf("WARNING: no memory map available.\n\n");
        }
    } else {
        kprintf("WARNING: no valid boot info\n");
    }

    PageAllocInit(boot);
    kprintf("OK: %u physical pages available.\n\n", PageAllocFreeCount());

    PagingInit();
    kprintf("OK: 4 GiB paging enabled with 4 KiB pages.\n\n");

    SchedInit();
    kprintf("OK: Scheduling Init Succesfull\n");

    SchedCreate("Worker", (u32)worker_task, (u32)&WorkerStack[TASK_STACK_SIZE]);
    SchedCreate("Shell",  (u32)shell_task,  (u32)&ShellStack[TASK_STACK_SIZE]);


    for (;;) {
        SchedYield();
        cpu_idle();
    }        
}
