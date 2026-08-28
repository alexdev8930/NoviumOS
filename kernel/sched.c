#include <novium/sched.h>

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

Task *SchedCreate(const char *Name, u32 Eip, u32 Esp) {
    u32 Index;
    Task *TaskItem;

    if (TaskCountValue >= SchedMaxTasks || Name == 0) {
        return 0;
    }

    TaskItem = 0;

    for (Index = 0; Index < SchedMaxTasks; Index++) {
        if (Tasks[Index].State == TaskDead) {
            TaskItem = &Tasks[Index];
            break;
        }
    }

    if (TaskItem == 0) {
        return 0;
    }

    TaskItem->Id = NextTaskId++;
    TaskItem->ParentId = CurrentTask != 0 ? CurrentTask->Id : 0;
    TaskItem->Eip = Eip;
    TaskItem->Esp = Esp;
    TaskItem->Ebp = Esp;
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

    if (SchedulerLocked != 0 || CurrentTask == 0) {
        return;
    }

    NextTask = SchedFindNext();

    if (NextTask == 0 || NextTask == CurrentTask) {
        return;
    }

    SchedQueueRemove(&ReadyQueue, NextTask);

    if (CurrentTask->State == TaskRunning) {
        CurrentTask->State = TaskReady;
        SchedQueueAdd(&ReadyQueue, CurrentTask);
    }

    NextTask->State = TaskRunning;
    NextTask->TimeUsed = 0;
    NextTask->Switches++;

    CurrentTask = NextTask;
    ContextSwitches++;
}

void SchedTick(struct registers *Regs) {
    if (CurrentTask == 0) {
        SchedInit();
    }

    if (Regs == 0 || SchedulerLocked != 0) {
        return;
    }

    SchedulerTicks++;
    CurrentTask->Runtime++;
    CurrentTask->TimeUsed++;

    if (CurrentTask->TimeUsed >= CurrentTask->TimeSlice) {
        CurrentTask->TimeUsed = 0;
    }
}

void SchedBlock(void) {
    if (CurrentTask == 0 || CurrentTask->Id == 0) {
        return;
    }

    SchedBlockTask(CurrentTask->Id);
}

void SchedBlockTask(u32 Id) {
    Task *TaskItem;

    TaskItem = SchedFind(Id);

    if (TaskItem == 0 || TaskItem->State == TaskDead) {
        return;
    }

    if (TaskItem->Id == 0) {
        return;
    }

    if (TaskItem->State == TaskBlocked) {
        return;
    }

    if (TaskItem == CurrentTask) {
        TaskItem->State = TaskBlocked;
        SchedYield();
        return;
    }

    if (TaskItem->State == TaskReady) {
        SchedQueueRemove(&ReadyQueue, TaskItem);
    }

    TaskItem->State = TaskBlocked;
    SchedQueueAdd(&BlockedQueue, TaskItem);
}

void SchedUnblock(Task *TaskItem) {
    if (TaskItem == 0 || TaskItem->State != TaskBlocked) {
        return;
    }

    SchedQueueRemove(&BlockedQueue, TaskItem);
    TaskItem->State = TaskReady;
    SchedQueueAdd(&ReadyQueue, TaskItem);
}

void SchedWakeTask(u32 Id) {
    Task *TaskItem;

    TaskItem = SchedFind(Id);

    if (TaskItem != 0) {
        SchedUnblock(TaskItem);
    }
}

void SchedExit(void) {
    Task *TaskItem;

    if (CurrentTask == 0 || CurrentTask->Id == 0) {
        return;
    }

    TaskItem = CurrentTask;

    SchedQueueRemove(&ReadyQueue, TaskItem);
    SchedQueueRemove(&BlockedQueue, TaskItem);

    TaskItem->State = TaskDead;
    SchedQueueAdd(&DeadQueue, TaskItem);

    if (TaskCountValue > 0) {
        TaskCountValue--;
    }

    CurrentTask = 0;
    SchedYield();
}

void SchedKill(u32 Id) {
    Task *TaskItem;

    TaskItem = SchedFind(Id);

    if (TaskItem == 0 || TaskItem->Id == 0) {
        return;
    }

    if (TaskItem == CurrentTask) {
        SchedExit();
        return;
    }

    SchedQueueRemove(&ReadyQueue, TaskItem);
    SchedQueueRemove(&BlockedQueue, TaskItem);

    TaskItem->State = TaskDead;
    SchedQueueAdd(&DeadQueue, TaskItem);

    if (TaskCountValue > 0) {
        TaskCountValue--;
    }
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
}

void SchedLock(void) {
    SchedulerLocked++;
}

void SchedUnlock(void) {
    if (SchedulerLocked > 0) {
        SchedulerLocked--;
    }
}
