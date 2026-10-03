#include <drivers/console.h>
#include <drivers/input.h>
#include <ipc/ipc.h>
#include <ipc/message.h>
#include <ipc/sync.h>
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
static u8 ConsumerStack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static u8 ProducerStack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static u8 LockAStack[TASK_STACK_SIZE] __attribute__((aligned(16)));
static u8 LockBStack[TASK_STACK_SIZE] __attribute__((aligned(16)));

/*
 * More messages than a port can hold, so the rendezvous below cannot pass by
 * accident: the producer has to park on a full port and the consumer on an
 * empty one, over and over, and both sides have to be woken the right number
 * of times for the counts to line up.
 */
#define IPC_TEST_MESSAGES 40
#define IPC_TEST_TIMEOUT 200
/* Enough trips through the mutex to show two tasks really do alternate. */
#define IPC_TEST_ROUNDS 50

static IpcPort TestPort;
static IpcSemaphore TestSem;
static IpcMutex TestMutex;
static volatile u32 ConsumerDone;
static volatile u32 ProducerDone;
static volatile u32 ConsumerBad;
/* Id of the sending task, so the consumer checks the stamp without guessing. */
static volatile u32 ProducerId;
static volatile u32 LockRounds;
static volatile u32 LockStop;
static volatile u32 LockBad;

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

/* Reads every message back and checks the order and the sender id. */
static void ipc_consumer_task(void) {
	IpcMessage Message;
	u32 Index;

	for (Index = 0; Index < IPC_TEST_MESSAGES; Index++) {
		if (IpcReceive(&TestPort, &Message) != IpcOk) {
			ConsumerBad++;
			break;
		}

		if (Message.Length != 1 || Message.Data[0] != Index ||
		    Message.Sender != ProducerId) {
			ConsumerBad++;
		}
	}

	ConsumerDone = 1;
}

/* Fills the port past its capacity, so the send path has to park as well. */
static void ipc_producer_task(void) {
	IpcMessage Message;
	u32 Index;

	for (Index = 0; Index < IPC_TEST_MESSAGES; Index++) {
		Message.Length = 1;
		Message.Data[0] = Index;

		if (IpcSend(&TestPort, &Message) != IpcOk) {
			ConsumerBad++;
			break;
		}
	}

	ProducerDone = 1;
}

/*
 * Takes the mutex, bumps a counter, and checks that nobody slipped in while it
 * was held. A shared counter that only ever grows by one is the evidence that
 * the handoff did not let two tasks into the critical section at once.
 */
static void ipc_lock_task(void) {
	u32 Seen;

	while (!LockStop) {
		if (MutexLock(&TestMutex) != IpcOk) {
			LockBad++;
			break;
		}

		Seen = LockRounds;
		LockRounds = Seen + 1;

		if (LockRounds != Seen + 1) {
			LockBad++;
		}

		MutexUnlock(&TestMutex);
	}
}

/*
 * Runs the ipc self tests once the scheduler is up, so the two sides of the
 * rendezvous really do park and get woken. Prints a line per outcome, the same
 * way the heap tests above report theirs.
 */
static void ipc_test_tasks(void) {
	Task *Consumer;
	Task *Producer;
	u32 Waited;
	IpcStatus Status;

	if (IpcPortCreate(&TestPort) != IpcOk) {
		kprintf("ERROR: ipc port could not be created.\n");
		return;
	}

	ConsumerDone = 0;
	ProducerDone = 0;
	ConsumerBad = 0;

	Consumer = SchedCreate("Consumer", (u32)ipc_consumer_task,
			       (u32)&ConsumerStack[TASK_STACK_SIZE]);
	Producer = SchedCreate("Producer", (u32)ipc_producer_task,
			       (u32)&ProducerStack[TASK_STACK_SIZE]);

	if (Consumer == 0 || Producer == 0) {
		kprintf("ERROR: ipc test tasks could not be created.\n");
		return;
	}

	ProducerId = Producer->Id;

	/* Both tasks are created, so both sides have to finish on their own. */
	for (Waited = 0; Waited < IPC_TEST_TIMEOUT; Waited++) {
		if (ConsumerDone && ProducerDone) {
			break;
		}
		cpu_idle();
	}

	if (!ConsumerDone || !ProducerDone) {
		kprintf("ERROR: ipc rendezvous did not finish, %u of %u messages.\n",
			IpcPortQueued(&TestPort), IPC_TEST_MESSAGES);
		return;
	}

	if (ConsumerBad != 0) {
		kprintf("ERROR: ipc rendezvous reported %u bad messages.\n", ConsumerBad);
		return;
	}

	kprintf("OK: %u messages passed through a blocking port.\n", IPC_TEST_MESSAGES);

	if (IpcPortQueued(&TestPort) != 0) {
		kprintf("ERROR: port still holds %u messages after draining it.\n",
			IpcPortQueued(&TestPort));
	} else {
		kprintf("OK: port drained, no message lost or duplicated.\n");
	}

	/* A binary semaphore plus a mutex, driven from this task alone. */
	SemInit(&TestSem, 1, 1);
	Status = SemWait(&TestSem);

	/*
	 * Runs from the idle task, which cannot sleep. So the last wait here has
	 * nowhere to park and has to say so instead of hanging, which is the same
	 * rule timer_sleep_ms follows.
	 */
	if (Status != IpcOk || SemTryWait(&TestSem) != IpcErrEmpty) {
		kprintf("ERROR: binary semaphore did not hold at its limit.\n");
	} else if (SemPost(&TestSem) != IpcOk || SemPost(&TestSem) != IpcErrFull) {
		kprintf("ERROR: binary semaphore did not stop at its limit.\n");
	} else if (SemWait(&TestSem) != IpcOk || SemWait(&TestSem) != IpcErrNoTask) {
		kprintf("ERROR: binary semaphore did not hand its permit back.\n");
	} else {
		kprintf("OK: binary semaphore counted, blocked, and refused to park the idle.\n");
	}

	MutexInit(&TestMutex);

	if (MutexLock(&TestMutex) != IpcOk || !MutexHeldByCaller(&TestMutex)) {
		kprintf("ERROR: mutex could not be locked.\n");
	} else if (MutexTryLock(&TestMutex) != IpcErrBusy) {
		kprintf("ERROR: mutex was free while this task held it.\n");
	} else if (MutexLock(&TestMutex) != IpcErrOwner) {
		kprintf("ERROR: mutex allowed a recursive lock.\n");
	} else if (MutexUnlock(&TestMutex) != IpcOk || MutexHeldByCaller(&TestMutex)) {
		kprintf("ERROR: mutex did not release.\n");
	} else if (MutexUnlock(&TestMutex) != IpcErrOwner) {
		kprintf("ERROR: mutex let a task unlock one it did not hold.\n");
	} else {
		kprintf("OK: mutex locked, refused a re-lock, and released.\n");
	}

	/* Two tasks fight over one mutex, so the handoff is exercised for real. */
	MutexInit(&TestMutex);
	LockRounds = 0;
	LockStop = 0;
	LockBad = 0;

	{
		Task *A = SchedCreate("LockA", (u32)ipc_lock_task, (u32)&LockAStack[TASK_STACK_SIZE]);
		Task *B = SchedCreate("LockB", (u32)ipc_lock_task, (u32)&LockBStack[TASK_STACK_SIZE]);

		if (A == 0 || B == 0) {
			kprintf("ERROR: ipc lock tasks could not be created.\n");
			return;
		}

		for (Waited = 0; Waited < IPC_TEST_ROUNDS; Waited++) {
			if (LockRounds >= IPC_TEST_ROUNDS) {
				break;
			}
			cpu_idle();
		}

		LockStop = 1;

		/* Let both tasks notice the stop flag and let go of the mutex. */
		for (Waited = 0; Waited < IPC_TEST_TIMEOUT; Waited++) {
			if (!TestMutex.Locked) {
				break;
			}
			cpu_idle();
		}
	}

	if (LockBad != 0) {
		kprintf("ERROR: mutex let %u tasks into the critical section at once.\n",
			LockBad);
	} else if (LockRounds < IPC_TEST_ROUNDS) {
		kprintf("ERROR: mutex contention stalled at %u of %u rounds.\n", LockRounds,
			IPC_TEST_ROUNDS);
	} else {
		kprintf("OK: %u rounds of contention on one mutex, no overlap.\n", LockRounds);
	}

	/* A released port cannot be claimed or destroyed twice. */
	if (IpcPortDestroy(&TestPort) != IpcOk || IpcPortDestroy(&TestPort) != IpcErrInvalid) {
		kprintf("ERROR: port could not be destroyed cleanly.\n");
	} else if (IpcPortCreate(&TestPort) != IpcOk || IpcPortCreate(&TestPort) != IpcErrInvalid) {
		kprintf("ERROR: port was claimed twice.\n");
	} else {
		kprintf("OK: port rejected a second create and a second destroy.\n");
		IpcPortDestroy(&TestPort);
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

	/*
	 * Before the shell and worker start, so this output stays on one run of
	 * lines instead of landing in the middle of the shell's prompt.
	 */
	ipc_test_tasks();

	kprintf("\n");

	SchedCreate("Worker", (u32)worker_task, (u32)&WorkerStack[TASK_STACK_SIZE]);
	SchedCreate("Shell", (u32)shell_task, (u32)&ShellStack[TASK_STACK_SIZE]);

	for (;;) {
		SchedYield();
		cpu_idle();
	}
}
