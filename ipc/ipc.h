#ifndef NOVIUM_IPC_H
#define NOVIUM_IPC_H

#include <novium/sched.h>
#include <novium/types.h>

/*
 * Inter-process communication between kernel tasks.
 *
 * Every object here is a fixed size value that lives in kernel memory, so a
 * task never has to allocate to talk to another one and there is nothing to
 * leak when a task dies. Ports, semaphores and mutexes are created by the
 * caller and may sit in the .bss, in a task's static area, or in a struct on
 * its own stack.
 */

/*
 * A wait queue never holds more than this, so a full queue is an error the
 * caller can see, instead of a task that blocks with nowhere to be woken.
 */
#define IpcMaxWaiters 16

typedef enum {
	IpcOk = 0,
	IpcErrInvalid, /* null or out of range argument */
	IpcErrFull,    /* a bounded queue or counter is already at its limit */
	IpcErrEmpty,   /* nothing to take */
	IpcErrBusy,    /* a non blocking call could not proceed right now */
	IpcErrOwner,   /* unlock from a task that does not hold the mutex */
	IpcErrNoTask   /* the caller cannot sleep and the call would have to */
} IpcStatus;

/* Id of the calling task, 0 for the idle task and for no task at all. The idle
 * task cannot sleep, so a call that would block must refuse to use it. */
u32 IpcSelfId(void);

/*
 * Wait queue, shared by ports, semaphores and mutexes. Entries are task ids
 * rather than Task pointers, because a Task descriptor is recycled once its
 * task dies while an id is not, so a stale entry can be spotted instead of
 * waking whichever task owns that slot now.
 */
typedef struct IpcWait {
	u32 Ids[IpcMaxWaiters];
	u32 Count;
} IpcWait;

/* Reports whether a parked task may stop waiting. Called with interrupts off,
 * so it may only read the object it is handed. */
typedef bool (*IpcReadyFn)(void *arg);

/*
 * Parks the calling task on Wait until Ready() turns true.
 *
 * The task registers itself while interrupts are off and only then sleeps, and
 * Ready() is re-checked under that same lock, so a waker that runs in between
 * either finds the task parked or is seen by the re-check afterwards. Waiting
 * is never lost and never left behind.
 *
 * Returns IpcErrNoTask when the caller is not a task that can sleep, and
 * IpcErrFull when the wait queue has no room for another entry.
 */
IpcStatus IpcPark(IpcWait *Wait, IpcReadyFn Ready, void *arg);

/*
 * Hands the object to the first waiter that is still alive, waking it if there
 * is one. Woken tasks are picked in no particular order. Safe to call with
 * interrupts on or off.
 */
void IpcWakeOne(IpcWait *Wait);

/*
 * Removes the first entry that still names a live task and returns its id, or
 * 0 when the queue holds nobody. Entries left by a killed task are dropped
 * along the way. For a mutex unlock, which needs the id before it can hand the
 * lock over. Interrupts must be off.
 */
u32 IpcWaitTakeLive(IpcWait *Wait);

#endif
