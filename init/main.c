#include <drivers/console.h>
#include <novium/debug.h>
#include <drivers/input.h>
#include <novium/boot_info.h>
#include <novium/init.h>
#include <novium/stdio.h>
#include <novium/sched.h>
#include <novium/timer.h>
#include <novium/cpu.h>

#define TASK_STACK_SIZE 4096
static u8 ShellStack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static u8 WorkerStack[TASK_STACK_SIZE] __attribute__((aligned(16)));

void shell_task(void) {
    printf("System ready. You can now type inside the console:\n");
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
    printf("NoviumOS\n\n");

    if (boot != NULL && boot->magic == BOOT_INFO_MAGIC) {
        printf("OK: Boot info valid.\nINFO: Boot drive: %d\n\n", boot->boot_drive_id);
    } else {
        printf("WARNING: no valid boot info\n");
    }

    SchedInit();
    printf("OK: Scheduling Init Succesfull\n");

    SchedCreate("Worker", (u32)worker_task, (u32)&WorkerStack[TASK_STACK_SIZE]);
    SchedCreate("Shell",  (u32)shell_task,  (u32)&ShellStack[TASK_STACK_SIZE]);


    for (;;) {
        SchedYield();
        cpu_idle();
    }        
}
