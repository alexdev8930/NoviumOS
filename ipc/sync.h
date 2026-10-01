#ifndef NOVIUM_IPC_SYNC_H
#define NOVIUM_IPC_SYNC_H

#include "ipc.h"

/*
 * Counting semaphores and mutexes. Both park a task on a shared wait queue and
 * hand the resource straight to the next waiter, so a wakeup cannot be taken
 * by a task that is not the one the queue picked.
 */

typedef struct IpcSemaphore {
	u32 Count; /* permits currently available */
	u32 Limit; /* ceiling for Count, 0 means there is no ceiling */
	IpcWait Wait;
} IpcSemaphore;

typedef struct IpcMutex {
	/* The owner alone cannot say whether the mutex is held, because the idle
	 * task is id 0 too, so held-ness is tracked apart from who holds it. */
	bool Locked;
	u32 Owner; /* task id of the holder, meaningless while Locked is false */
	IpcWait Wait;
} IpcMutex;

/*
 * Semaphore.
 *
 * A Limit of 0 is an unbounded counter, and a Limit of 1 turns it into a binary
 * semaphore, which is what most of the kernel's own locks will want.
 */
void SemInit(IpcSemaphore *Sem, u32 Initial, u32 Limit);

/* Takes a permit, blocking while none is available. */
IpcStatus SemWait(IpcSemaphore *Sem);

/* Takes a permit only when one is free. Never blocks. */
IpcStatus SemTryWait(IpcSemaphore *Sem);

/* Returns a permit and wakes one waiter. Fails when the ceiling is reached. */
IpcStatus SemPost(IpcSemaphore *Sem);

u32 SemCount(const IpcSemaphore *Sem);

/*
 * Mutex.
 *
 * Not recursive: locking a mutex the caller already holds reports IpcErrOwner
 * instead of parking on a lock nobody else is able to open.
 */
void MutexInit(IpcMutex *Mutex);

/* Takes the mutex, blocking while another task holds it. */
IpcStatus MutexLock(IpcMutex *Mutex);

/* Takes the mutex only when it is free. Never blocks. */
IpcStatus MutexTryLock(IpcMutex *Mutex);

/* Releases the mutex and hands it to the first waiter. */
IpcStatus MutexUnlock(IpcMutex *Mutex);

/* True when the calling task is the one holding the mutex. */
bool MutexHeldByCaller(const IpcMutex *Mutex);

#endif
