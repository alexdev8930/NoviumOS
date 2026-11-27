#include <asm/cpu.h>
#include <novium/cpu.h>
#include <novium/sched.h>

void cpu_idle(void) {
	cpu_hlt();
}

void cpu_disable_irqs(void) {
	cpu_cli();
}

void cpu_enable_irqs(void) {
	cpu_sti();
}

bool cpu_irqs_enabled(void) {
	return cpu_irq_enabled();
}

static void SchedExitStub(void (*Entry)(void)) {
	cpu_enable_irqs();
	Entry();
	SchedExit();
}

void SchedInitStack(Task *TaskItem, void (*Entry)(void), u32 StackTop) {
	u32 *sp = (u32 *)StackTop;

	*--sp = (u32)Entry;			/* arg[0] for SchedExitStub         */
	*--sp = 0;					/* SchedExitStub's fake return addr */
	*--sp = (u32)SchedExitStub; /* SchedSwitch `ret` jumps here     */
	*--sp = 0x202;				/* eflags (IF set)                  */
	*--sp = 0;					/* ebp                              */
	*--sp = 0;					/* ebx                              */
	*--sp = 0;					/* esi                              */
	*--sp = 0;					/* edi                              */

	TaskItem->Esp = (u32)sp;
}
