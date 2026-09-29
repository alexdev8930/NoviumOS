#include <drivers/console.h>
#include <drivers/input.h>
#include <novium/boot_info.h>
#include <novium/cpu.h>
#include <novium/debug.h>
#include <novium/init.h>
#include <novium/mm.h>
#include <novium/sched.h>
#include <novium/stdio.h>
#include <novium/timer.h>

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

		console_line_echo = true;
		console_putchar((char)key);
		console_line_echo = false;

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
			kprintf("OK: Memory map available.\n");
		} else {
			kprintf("WARNING: no memory map available.\n");
		}
	} else {
		kprintf("WARNING: no valid boot info\n");
	}

	PageAllocInit(boot);
	HeapInit();
	kprintf("OK: %u physical pages available.\n", PageAllocFreeCount());

	PagingInit();
	kprintf("OK: 4 GiB paging enabled with 4 KiB pages.\n");

	u32 HeapPagesBefore = PageAllocFreeCount();
	void *HeapTestMemory = kmalloc(PAGE_SIZE + 1);
	void *MergeA;
	void *MergeB;
	void *MergeBig;
	u32 PagesAfterMerge;

	if (HeapTestMemory == 0) {
		kprintf("ERROR: multi-page kmalloc test failed.\n");
	} else {
		kprintf("OK: kmalloc(PAGE_SIZE + 1) returned 0x%x.\n", (u32)HeapTestMemory);

		if (PageAllocFreeCount() == HeapPagesBefore) {
			kprintf("OK: kmalloc reused a free block, no pages taken.\n");
		} else {
			kprintf("WARNING: kmalloc took new pages, free list was empty.\n");
		}

		kfree(HeapTestMemory);
	}

	/* Two 2 page blocks, freed one after another. */
	MergeA = kmalloc(PAGE_SIZE * 2);
	MergeB = kmalloc(PAGE_SIZE * 2);

	if (MergeA == 0 || MergeB == 0) {
		kprintf("ERROR: merge test could not allocate two blocks.\n");
	} else {
		kprintf("OK: two 2 page blocks allocated at 0x%x and 0x%x.\n", (u32)MergeA,
			(u32)MergeB);

		kfree(MergeA);
		kfree(MergeB);

		MergeBig = kmalloc(PAGE_SIZE * 4);

		if (MergeBig != 0) {
			kprintf("OK: two freed blocks coalesced into one 4 page block.\n");
			kfree(MergeBig);
		} else {
			kprintf("WARNING: two freed blocks did not coalesce.\n");
		}
	}

	/* The heap gave pages back, so the count should be higher than before. */
	PagesAfterMerge = PageAllocFreeCount();
	if (PagesAfterMerge > HeapPagesBefore) {
		kprintf("OK: heap returned pages to the page allocator.\n");
	} else if (PagesAfterMerge == HeapPagesBefore) {
		kprintf("OK: heap kept its pages, nothing large enough to return.\n");
	} else {
		kprintf("ERROR: heap lost pages, %u before and %u after.\n", HeapPagesBefore,
			PagesAfterMerge);
	}

	SchedInit();
	kprintf("OK: Scheduling Init Succesfull\n\n");

	SchedCreate("Worker", (u32)worker_task, (u32)&WorkerStack[TASK_STACK_SIZE]);
	SchedCreate("Shell", (u32)shell_task, (u32)&ShellStack[TASK_STACK_SIZE]);

	for (;;) {
		SchedYield();
		cpu_idle();
	}
}
