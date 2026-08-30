#ifndef NOVIUM_SCHED_H
#define NOVIUM_SCHED_H

#include <novium/types.h>
#include <asm/irq.h>

#define SchedMaxTasks 32
#define SchedNameLength 32

typedef enum {
    TaskDead = 0,
    TaskReady,
    TaskRunning,
    TaskBlocked
} TaskState;

typedef enum {
    SchedRoundRobin = 0,
    SchedPriority
} SchedPolicy;

typedef struct Task {
    u32 Id;
    u32 ParentId;

    u32 Eip;
    u32 Esp;
    u32 Ebp;

    u32 Priority;
    u32 TimeSlice;
    u32 TimeUsed;

    u32 Runtime;
    u32 Switches;

    TaskState State;

    char Name[SchedNameLength];

    struct Task *Next;
} Task;

typedef struct {
    u32 TotalTasks;
    u32 RunningTasks;
    u32 ReadyTasks;
    u32 BlockedTasks;
    u32 DeadTasks;

    u32 SchedulerTicks;
    u32 ContextSwitches;
} SchedStats;

void SchedInit(void);

/* Task management */
Task *SchedCreate(const char *Name, u32 Eip, u32 Esp);
Task *SchedCurrent(void);
Task *SchedFind(u32 Id);
u32 SchedTaskCount(void);

void SchedYield(void);
void SchedTick(struct registers *Regs);

void SchedBlock(void);
void SchedBlockTask(u32 Id);
void SchedUnblock(Task *TaskItem);
void SchedWakeTask(u32 Id);

void SchedExit(void);
void SchedKill(u32 Id);

void SchedSetPolicy(SchedPolicy Policy);
SchedPolicy SchedGetPolicy(void);

void SchedSetPriority(Task *TaskItem, u32 Priority);
u32 SchedGetPriority(Task *TaskItem);

void SchedGetStats(SchedStats *Stats);

void SchedLock(void);
void SchedUnlock(void);

void SchedSwitch(u32 *OldEsp, u32 NewEsp);

#endif

