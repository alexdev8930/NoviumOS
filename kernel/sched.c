#include <novium/sched.h>
#include <novium/cpu.h>

static Task Tasks[SchedMaxTasks];
static Task *ReadyQueue = 0;
static Task *BlockedQueue = 0;
static Task *DeadQueue = 0;
static Task *CurrentTask = 0;

static SchedPolicy CurrentPolicy = SchedRoundRobin;

static u32 TaskCountValue = 0;
static u32 NextTaskId = 0;
static u32 SchedulerTicks = 0;
static u32 ContextSwitches = 0;
static u32 SchedulerLocked = 0;
static u32 PreemptPending = 0;

/* Helper: saves interrupt status and disables interrupts */
static inline u32 SchedLockIrq(void) {
    u32 was_enabled = cpu_irqs_enabled() ? 1 : 0;
    cpu_disable_irqs();
    return was_enabled;
}

/* Helper: restores interrupts only if they were originally on */
static inline void SchedUnlockIrq(u32 was_enabled) {
    if (was_enabled) {
        cpu_enable_irqs();
    }
}

/* appends a node to the absolute tail of a list */
static void SchedQueueAdd(Task **Queue, Task *TaskItem) {
    TaskItem->Next = 0;

    if (*Queue == 0) {
        *Queue = TaskItem;
        return;
    }

    Task *Current = *Queue;

    while (Current->Next != 0) {
        Current = Current->Next;
    }

    Current->Next = TaskItem;
}

/* extracts a target node and bridges neighboring elements */
static void SchedQueueRemove(Task **Queue, Task *TaskItem) {
    Task *Current;
    Task *Previous;

    if (Queue == 0 || *Queue == 0 || TaskItem == 0) {
        return;
    }

    Current = *Queue;
    Previous = 0;

    while (Current != 0) {
        if (Current == TaskItem) {
            if (Previous == 0) {
                *Queue = Current->Next;
            } else {
                Previous->Next = Current->Next;
            }

            Current->Next = 0;
            return;
        }

        Previous = Current;
        Current = Current->Next;
    }
}

static Task *SchedFindRoundRobin(void) {
    Task *TaskItem;

    if (CurrentTask == 0) {
        return ReadyQueue;
    }

    TaskItem = ReadyQueue;

    while (TaskItem != 0) {
        if (TaskItem->Id > CurrentTask->Id) {
            return TaskItem;
        }

        TaskItem = TaskItem->Next;
    }

    return ReadyQueue;
}

static Task *SchedFindPriority(void) {
    Task *Current;
    Task *Best;

    Current = ReadyQueue;
    Best = 0;

    while (Current != 0) {
        if (Best == 0 || Current->Priority > Best->Priority) {
            Best = Current;
        }

        Current = Current->Next;
    }

    return Best;
}

static Task *SchedFindNext(void) {
    if (CurrentPolicy == SchedPriority) {
        return SchedFindPriority();
    }

    return SchedFindRoundRobin();
}

static void SchedResetTask(Task *TaskItem) {
    TaskItem->Id = 0;
    TaskItem->ParentId = 0;
    TaskItem->Eip = 0;
    TaskItem->Esp = 0;
    TaskItem->Ebp = 0;
    TaskItem->Priority = 1;
    TaskItem->TimeSlice = 10;
    TaskItem->TimeUsed = 0;
    TaskItem->Runtime = 0;
    TaskItem->Switches = 0;
    TaskItem->WakeTick = 0;
    TaskItem->State = TaskDead;
    TaskItem->Name[0] = 0;
    TaskItem->Next = 0;
}

void SchedInit(void) {
    u32 Index;
    Task *IdleTask;

    ReadyQueue = 0;
    BlockedQueue = 0;
    DeadQueue = 0;
    CurrentTask = 0;

    TaskCountValue = 0;
    NextTaskId = 0;
    SchedulerTicks = 0;
    ContextSwitches = 0;
    SchedulerLocked = 0;
    PreemptPending = 0;

    for (Index = 0; Index < SchedMaxTasks; Index++) {
        SchedResetTask(&Tasks[Index]);
    }

    IdleTask = &Tasks[0];

    IdleTask->Id = NextTaskId++;
    IdleTask->ParentId = 0;
    IdleTask->Priority = 0;
    IdleTask->TimeSlice = 10;
    IdleTask->State = TaskRunning;

    IdleTask->Name[0] = 'I';
    IdleTask->Name[1] = 'd';
    IdleTask->Name[2] = 'l';
    IdleTask->Name[3] = 'e';
    IdleTask->Name[4] = 0;

    CurrentTask = IdleTask;
    TaskCountValue = 1;
}

static void SchedExitStub(void (*Entry)(void)) {
    cpu_enable_irqs();
    Entry();     
    SchedExit();   
}

void SchedInitStack(Task *TaskItem, void (*Entry)(void), u32 StackTop) {
    u32 *sp = (u32 *)StackTop;

    /*
     * This layout must mirror SchedSwitch's epilogue exactly, reading up from
     * the final esp:
     *
     *   popl %edi   popl %esi   popl %ebx   popl %ebp   popfl   ret
     *
     * so `ret` lands on SchedExitStub with Entry as its first argument.
     * 0x202 is a valid eflags for a fresh task: bit 1 is always set and bit 9
     * is the interrupt flag, so a brand new task starts with interrupts on.
     */
    *--sp = (u32)Entry;            /* arg[0] for SchedExitStub          */
    *--sp = 0;                     /* SchedExitStub's fake return addr  */
    *--sp = (u32)SchedExitStub;    /* SchedSwitch `ret` jumps here       */
    *--sp = 0x202;                 /* eflags (if set) */
    *--sp = 0;                     /* ebp */
    *--sp = 0;                     /* ebx */
    *--sp = 0;                     /* esi */
    *--sp = 0;                     /* edi */

    TaskItem->Esp = (u32)sp;
}

/* finds an empty descriptor and builds a new process node */
Task *SchedCreate(const char *Name, u32 Eip, u32 Esp) {
    u32 Index;
    Task *TaskItem;

    if (TaskCountValue >= SchedMaxTasks || Name == 0) {
        return 0;
    }

    u32 was_enabled = SchedLockIrq();

    TaskItem = 0;

    for (Index = 0; Index < SchedMaxTasks; Index++) {
        if (Tasks[Index].State == TaskDead) {
            TaskItem = &Tasks[Index];
            break;
        }
    }

    if (TaskItem == 0) {
        SchedUnlockIrq(was_enabled);
        return 0;
    }

    SchedQueueRemove(&DeadQueue, TaskItem);
    
    TaskItem->Id = NextTaskId++;
    TaskItem->ParentId = CurrentTask != 0 ? CurrentTask->Id : 0;

    SchedInitStack(TaskItem, (void (*)(void))Eip, Esp);

    TaskItem->Eip = Eip;
    TaskItem->Ebp = TaskItem->Esp; 
    TaskItem->Priority = 1;
    TaskItem->TimeSlice = 10;
    TaskItem->TimeUsed = 0;
    TaskItem->Runtime = 0;
    TaskItem->Switches = 0;
    TaskItem->State = TaskReady;

    u32 NameIndex;
    for (NameIndex = 0; NameIndex < SchedNameLength - 1 && Name[NameIndex] != 0; NameIndex++) {
        TaskItem->Name[NameIndex] = Name[NameIndex];
    }
    TaskItem->Name[NameIndex] = 0;

    SchedQueueAdd(&ReadyQueue, TaskItem);
    TaskCountValue++;

    SchedUnlockIrq(was_enabled);
    return TaskItem;
}

Task *SchedCurrent(void) {
    return CurrentTask;
}

Task *SchedFind(u32 Id) {
    u32 Index;

    for (Index = 0; Index < SchedMaxTasks; Index++) {
        if (Tasks[Index].State != TaskDead && Tasks[Index].Id == Id) {
            return &Tasks[Index];
        }
    }

    return 0;
}

u32 SchedTaskCount(void) {
    return TaskCountValue;
}

void SchedYield(void) {
    Task *NextTask;
    Task *PrevTask;
    u32 ScratchEsp;

    u32 was_enabled = SchedLockIrq();

    if (SchedulerLocked != 0) {
        SchedUnlockIrq(was_enabled);
        return;
    }

    NextTask = SchedFindNext();

    if (NextTask == 0 || NextTask == CurrentTask) {
        SchedUnlockIrq(was_enabled);
        return;
    }

    PrevTask = CurrentTask;

    SchedQueueRemove(&ReadyQueue, NextTask);

    if (PrevTask != 0 && PrevTask->State == TaskRunning) {
        PrevTask->State = TaskReady;
        SchedQueueAdd(&ReadyQueue, PrevTask);
    }

    NextTask->State = TaskRunning;
    NextTask->TimeUsed = 0;
    NextTask->Switches++;

    CurrentTask = NextTask;
    ContextSwitches++;

    /*
     * The switch stays atomic on purpose: SchedSwitch's `popfl` re-installs
     * whichever interrupt flag belongs to the task being resumed, so a task
     * that was preempted out of an irq handler comes back with interrupts
     * still off (mid handler), and a task that yielded normally comes back
     * with them on.
     */
    SchedSwitch(PrevTask != 0 ? &PrevTask->Esp : &ScratchEsp, NextTask->Esp);

    SchedUnlockIrq(was_enabled);
}

void SchedTick(struct registers *Regs) {
    if (Regs == 0 || SchedulerLocked != 0) {
        return;
    }

    SchedulerTicks++;

    if (CurrentTask == 0) {
        return;
    }

    CurrentTask->Runtime++;
    CurrentTask->TimeUsed++;

    /*
     * Only ask for the preemption here. This runs while the pic still holds
     * the timer line in service, because isr_dispatch acknowledges the pic
     * after the handler returns. Switching tasks right now would suspend this
     * handler with its eoi still pending, which mutes every interrupt on that
     * controller, and the first task that then halts would never wake up.
     * SchedPreempt() performs the switch from the epilogue, after the eoi.
     */
    if (CurrentTask->TimeUsed >= CurrentTask->TimeSlice) {
        CurrentTask->TimeUsed = 0;
        PreemptPending = 1;
    }
}

void SchedPreempt(void) {
    if (PreemptPending == 0) {
        return;
    }

    /*
     * While scheduling is locked the request is kept, not dropped, so it is
     * honoured by the next interrupt epilogue instead of being lost.
     */
    if (SchedulerLocked != 0) {
        return;
    }

    PreemptPending = 0;

    SchedYield();
}

void SchedBlock(void) {
    if (CurrentTask == 0 || CurrentTask->Id == 0) {
        return;
    }

    SchedBlockTask(CurrentTask->Id);
}

void SchedBlockTask(u32 Id) {
    u32 was_enabled = SchedLockIrq();

    Task *TaskItem = SchedFind(Id);

    if (TaskItem == 0 || TaskItem->State == TaskDead || TaskItem->Id == 0 || TaskItem->State == TaskBlocked) {
        SchedUnlockIrq(was_enabled);
        return;
    }

    if (TaskItem == CurrentTask) {
        SchedQueueRemove(&ReadyQueue, TaskItem);
        TaskItem->State = TaskBlocked;
        SchedQueueAdd(&BlockedQueue, TaskItem);

        SchedUnlockIrq(was_enabled);
        SchedYield();

        was_enabled = SchedLockIrq();
        if (TaskItem->State == TaskBlocked) {
            SchedQueueRemove(&BlockedQueue, TaskItem);
            TaskItem->State = TaskRunning;
        }
        SchedUnlockIrq(was_enabled);
        return;
    }

    if (TaskItem->State == TaskReady) {
        SchedQueueRemove(&ReadyQueue, TaskItem);
    }

    TaskItem->State = TaskBlocked;
    SchedQueueAdd(&BlockedQueue, TaskItem);

    SchedUnlockIrq(was_enabled);
}

void SchedUnblock(Task *TaskItem) {
    if (TaskItem == 0 || TaskItem->State != TaskBlocked) {
        return;
    }

    u32 was_enabled = SchedLockIrq();
    SchedQueueRemove(&BlockedQueue, TaskItem);
    TaskItem->State = TaskReady;
    SchedQueueAdd(&ReadyQueue, TaskItem);
    SchedUnlockIrq(was_enabled);
}

void SchedWakeTask(u32 Id) {
    Task *TaskItem = SchedFind(Id);
    if (TaskItem != 0) {
        SchedUnblock(TaskItem);
    }
}

void SchedSleepUntil(u32 WakeTick) {
    Task *TaskItem = CurrentTask;

    if (TaskItem == 0 || TaskItem->Id == 0) {
        return;
    }

    TaskItem->WakeTick = WakeTick;
    SchedBlockTask(TaskItem->Id);
    TaskItem->WakeTick = 0;
}

u32 SchedWakeExpired(u32 NowTick) {
    u32 was_enabled = SchedLockIrq();
    Task *TaskItem = BlockedQueue;
    u32 Woken = 0;

    while (TaskItem != 0) {
        Task *Next = TaskItem->Next;

        if (TaskItem->WakeTick != 0 &&
            (s32)(NowTick - TaskItem->WakeTick) >= 0) {
            TaskItem->WakeTick = 0;
            SchedQueueRemove(&BlockedQueue, TaskItem);
            TaskItem->State = TaskReady;
            SchedQueueAdd(&ReadyQueue, TaskItem);
            Woken++;
        }

        TaskItem = Next;
    }

    SchedUnlockIrq(was_enabled);
    return Woken;
}

u32 SchedHasRunnable(void) {
    u32 was_enabled = SchedLockIrq();
    Task *TaskItem = ReadyQueue;

    while (TaskItem != 0) {
        if (TaskItem != CurrentTask) {
            SchedUnlockIrq(was_enabled);
            return 1;
        }

        TaskItem = TaskItem->Next;
    }

    SchedUnlockIrq(was_enabled);
    return 0;
}

void SchedExit(void) {
    Task *TaskItem;

    if (CurrentTask == 0 || CurrentTask->Id == 0) {
        return;
    }

    u32 was_enabled = SchedLockIrq();

    TaskItem = CurrentTask;

    SchedQueueRemove(&ReadyQueue, TaskItem);
    SchedQueueRemove(&BlockedQueue, TaskItem);

    SchedResetTask(TaskItem);
    SchedQueueAdd(&DeadQueue, TaskItem);

    if (TaskCountValue > 0) {
        TaskCountValue--;
    }

    CurrentTask = 0;
    SchedUnlockIrq(was_enabled);

    SchedYield();

    if (CurrentTask == 0) {
        CurrentTask = &Tasks[0];
        CurrentTask->State = TaskRunning;
    }
}

void SchedKill(u32 Id) {
    Task *TaskItem;

    if (Id == 0) {
        return;
    }

    if (CurrentTask != 0 && CurrentTask->Id == Id) {
        SchedExit();
        return;
    }

    u32 was_enabled = SchedLockIrq();

    TaskItem = SchedFind(Id);

    if (TaskItem == 0 || TaskItem->Id == 0) {
        SchedUnlockIrq(was_enabled);
        return;
    }

    SchedQueueRemove(&ReadyQueue, TaskItem);
    SchedQueueRemove(&BlockedQueue, TaskItem);

    SchedResetTask(TaskItem);
    SchedQueueAdd(&DeadQueue, TaskItem);

    if (TaskCountValue > 0) {
        TaskCountValue--;
    }

    SchedUnlockIrq(was_enabled);
}

void SchedSetPolicy(SchedPolicy Policy) {
    if (Policy != SchedRoundRobin && Policy != SchedPriority) {
        return;
    }

    CurrentPolicy = Policy;
}

SchedPolicy SchedGetPolicy(void) {
    return CurrentPolicy;
}

void SchedSetPriority(Task *TaskItem, u32 Priority) {
    if (TaskItem == 0 || TaskItem->State == TaskDead) {
        return;
    }

    if (Priority > 255) {
        Priority = 255;
    }

    TaskItem->Priority = Priority;
}

u32 SchedGetPriority(Task *TaskItem) {
    if (TaskItem == 0) {
        return 0;
    }

    return TaskItem->Priority;
}

void SchedGetStats(SchedStats *Stats) {
    u32 Index;

    if (Stats == 0) {
        return;
    }

    u32 was_enabled = SchedLockIrq();

    Stats->TotalTasks = TaskCountValue;
    Stats->RunningTasks = 0;
    Stats->ReadyTasks = 0;
    Stats->BlockedTasks = 0;
    Stats->DeadTasks = 0;
    Stats->SchedulerTicks = SchedulerTicks;
    Stats->ContextSwitches = ContextSwitches;

    for (Index = 0; Index < SchedMaxTasks; Index++) {
        switch (Tasks[Index].State) {
            case TaskRunning:
                Stats->RunningTasks++;
                break;

            case TaskReady:
                Stats->ReadyTasks++;
                break;

            case TaskBlocked:
                Stats->BlockedTasks++;
                break;

            case TaskDead:
                Stats->DeadTasks++;
                break;
        }
    }

    SchedUnlockIrq(was_enabled);
}

void SchedLock(void) {
    SchedulerLocked++;
}

void SchedUnlock(void) {
    if (SchedulerLocked > 0) {
        SchedulerLocked--;
    }
}
