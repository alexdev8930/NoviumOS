#include "ipc.h"
#include <novium/cpu.h>

u32 IpcSelfId(void) {
	Task *Self = SchedCurrent();

	if (Self == 0) {
		return 0;
	}

	return Self->Id;
}

/*
 * Parks the caller on Wait. Needs interrupts off for the whole queue
 * bookkeeping, so the helpers below are the only thing allowed to touch a
 * queue while it is asleep, and every waker goes through IpcWakeOne.
 */
static bool IpcWaitAdd(IpcWait *Wait, u32 Id) {
	u32 Index;

	if (Wait->Count >= IpcMaxWaiters) {
		return false;
	}

	for (Index = 0; Index < Wait->Count; Index++) {
		if (Wait->Ids[Index] == Id) {
			return true; /* already parked, a second entry would only confuse a wake */
		}
	}

	Wait->Ids[Wait->Count] = Id;
	Wait->Count++;
	return true;
}

static void IpcWaitRemove(IpcWait *Wait, u32 Id) {
	u32 Index;

	for (Index = 0; Index < Wait->Count; Index++) {
		if (Wait->Ids[Index] == Id) {
			/* Not worth keeping order for, and the queue is short. */
			Wait->Ids[Index] = Wait->Ids[Wait->Count - 1];
			Wait->Count--;
			return;
		}
	}
}

/*
 * True when Id still names a task that is alive and can be woken. A killed
 * task has its id cleared and a recycled descriptor gets a fresh id, so a
 * single lookup is enough to rule out both.
 */
static bool IpcWaitIsLive(u32 Id) {
	return Id != 0 && SchedFind(Id) != 0;
}

u32 IpcWaitTakeLive(IpcWait *Wait) {
	u32 Index;

	for (Index = 0; Index < Wait->Count; Index++) {
		u32 Id = Wait->Ids[Index];

		IpcWaitRemove(Wait, Id);

		if (Id != 0 && IpcWaitIsLive(Id)) {
			return Id;
		}
	}

	return 0;
}

IpcStatus IpcPark(IpcWait *Wait, IpcReadyFn Ready, void *arg) {
	u32 Self = IpcSelfId();

	if (Self == 0) {
		/* The idle task cannot sleep, so it must not pretend it did. */
		return IpcErrNoTask;
	}

	for (;;) {
		bool was_enabled = cpu_irqs_enabled();

		cpu_disable_irqs();

		if (Ready(arg)) {
			if (was_enabled) {
				cpu_enable_irqs();
			}
			return IpcOk;
		}

		if (!IpcWaitAdd(Wait, Self)) {
			if (was_enabled) {
				cpu_enable_irqs();
			}
			return IpcErrFull;
		}

		/*
		 * The lock stays held from here into SchedBlock on purpose. Being on
		 * the queue and being blocked have to happen as one step: if the lock
		 * were dropped in between, a waker could take the entry, find the task
		 * still runnable, and its SchedWakeTask would do nothing, leaving the
		 * task to park afterwards with nobody left to wake it.
		 *
		 * Nothing else can run while the lock is held, since a task only
		 * changes at a SchedYield and those are reached from the interrupt
		 * epilogue. So the entry is on the queue before we are marked blocked,
		 * and Ready() was checked after the last thing that could set it.
		 *
		 * SchedBlock leaves the lock held for its caller: it restores the task
		 * eflags on the way back, which are clear here.
		 */
		SchedBlock();

		/* Woken, or the scheduler found nothing to run and undid the block. */
		cpu_disable_irqs();
		IpcWaitRemove(Wait, Self);
		if (was_enabled) {
			cpu_enable_irqs();
		}
	}
}

void IpcWakeOne(IpcWait *Wait) {
	u32 Id;
	bool was_enabled = cpu_irqs_enabled();

	cpu_disable_irqs();
	Id = IpcWaitTakeLive(Wait);
	if (was_enabled) {
		cpu_enable_irqs();
	}

	if (Id != 0) {
		SchedWakeTask(Id);
	}
}