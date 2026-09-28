.section .text
.global SchedSwitch

# void SchedSwitch(u32 *OldEsp, u32 NewEsp);
#
# The interrupt flag (eflags.if) is part of the saved context.  Without it a
# task that is preempted from inside an irq handler (cpu.if == 0) would hand
# that cleared flag to whichever task it resumes, and a task that then halts
# would never be woken again.
SchedSwitch:
    pushfl
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    # save old task's stack pointer, then flip ESP to the new task's stack
    # (5 dwords pushed above, plus the return address, so the args sit at 24/28)
    movl 24(%esp), %eax
    movl %esp, (%eax)
    movl 28(%esp), %esp

    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    popfl
    ret