#include "sync.h"
#include <novium/cpu.h>

/*
 * Semaphore.
 *
 * A Limit of 0 is an unbounded counter. Setting one turns it into a binary
 * semaphore, which is what the kernel's own locks will mostly want.
 */
void SemInit(IpcSemaphore *Sem, u32 Initial, u32 Limit) {
	if (Sem == 0) {
		return;
	}

	if (Limit != 0 && Initial > Limit) {
		/* Start at the ceiling rather than above it, so Count <= Limit holds. */
		Initial = Limit;
	}

	Sem->Count = Initial;
	Sem->Limit = Limit;
	Sem->Wait.Count = 0;
}

/* True while a permit is free, which is what a parked sender is waiting for. */
static bool SemHasPermit(void *arg) {
	IpcSemaphore *Sem = (IpcSemaphore *)arg;

	return Sem->Count > 0;
}

IpcStatus SemTryWait(IpcSemaphore *Sem) {
	bool was_enabled;
	IpcStatus Status = IpcOk;

	if (Sem == 0) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Sem->Count == 0) {
		Status = IpcErrEmpty;
	} else {
		Sem->Count--;
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	return Status;
}

IpcStatus SemWait(IpcSemaphore *Sem) {
	if (Sem == 0) {
		return IpcErrInvalid;
	}

	for (;;) {
		IpcStatus Status = SemTryWait(Sem);

		if (Status != IpcErrEmpty) {
			return Status;
		}

		/* No permit. Wait for a post, then try again. */
		Status = IpcPark(&Sem->Wait, SemHasPermit, Sem);
		if (Status != IpcOk) {
			return Status;
		}
	}
}

IpcStatus SemPost(IpcSemaphore *Sem) {
	bool was_enabled;
	IpcStatus Status = IpcOk;

	if (Sem == 0) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Sem->Limit != 0 && Sem->Count >= Sem->Limit) {
		/* At the ceiling. Posting here would let the count run away. */
		Status = IpcErrFull;
	} else {
		Sem->Count++;
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	if (Status == IpcOk) {
		IpcWakeOne(&Sem->Wait);
	}

	return Status;
}

u32 SemCount(const IpcSemaphore *Sem) {
	if (Sem == 0) {
		return 0;
	}

	return Sem->Count;
}

/*
 * Mutex.
 *
 * Not recursive. Locking a mutex the caller already holds reports IpcErrOwner
 * rather than parking on a lock nobody else is able to open, which would hang
 * the task until the scheduler undid the block.
 */
void MutexInit(IpcMutex *Mutex) {
	if (Mutex == 0) {
		return;
	}

	Mutex->Locked = false;
	Mutex->Owner = 0;
	Mutex->Wait.Count = 0;
}

bool MutexHeldByCaller(const IpcMutex *Mutex) {
	if (Mutex == 0 || !Mutex->Locked) {
		return false;
	}

	return Mutex->Owner == IpcSelfId();
}

/* True while the mutex is free, which is what a parked locker waits for. */
static bool MutexIsFree(void *arg) {
	IpcMutex *Mutex = (IpcMutex *)arg;

	return !Mutex->Locked;
}

IpcStatus MutexTryLock(IpcMutex *Mutex) {
	bool was_enabled;
	IpcStatus Status = IpcOk;

	if (Mutex == 0) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (Mutex->Locked) {
		Status = IpcErrBusy;
	} else {
		Mutex->Locked = true;
		Mutex->Owner = IpcSelfId();
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	return Status;
}

IpcStatus MutexLock(IpcMutex *Mutex) {
	if (Mutex == 0) {
		return IpcErrInvalid;
	}

	if (MutexHeldByCaller(Mutex)) {
		return IpcErrOwner;
	}

	for (;;) {
		IpcStatus Status = MutexTryLock(Mutex);

		if (Status == IpcErrBusy && MutexHeldByCaller(Mutex)) {
			/*
			 * An unlock passed the lock straight to us while we were parked,
			 * so the mutex is ours even though it still reads as taken.
			 */
			return IpcOk;
		}

		if (Status != IpcErrBusy) {
			return Status;
		}

		/* Held. Wait for the owner to release it, then try again. */
		Status = IpcPark(&Mutex->Wait, MutexIsFree, Mutex);
		if (Status != IpcOk) {
			return Status;
		}
	}
}

IpcStatus MutexUnlock(IpcMutex *Mutex) {
	bool was_enabled;
	u32 Next = 0;
	IpcStatus Status = IpcOk;

	if (Mutex == 0) {
		return IpcErrInvalid;
	}

	was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	if (!Mutex->Locked || Mutex->Owner != IpcSelfId()) {
		/*
		 * Only the holder may open the lock. Allowing anyone through would let a
		 * task release a mutex it never took and hand it to a waiter at the
		 * same time.
		 */
		Status = IpcErrOwner;
	} else {
		/*
		 * Take a waiter now and pass the lock straight to it, rather than
		 * clearing the owner and letting a task that is merely ready take it
		 * first. That keeps the wakeup and the lock in step.
		 */
		Next = IpcWaitTakeLive(&Mutex->Wait);

		if (Next != 0) {
			Mutex->Owner = Next;
		} else {
			Mutex->Locked = false;
			Mutex->Owner = 0;
		}
	}

	if (was_enabled) {
		cpu_enable_irqs();
	}

	if (Status == IpcOk && Next != 0) {
		SchedWakeTask(Next);
	}

	return Status;
}
