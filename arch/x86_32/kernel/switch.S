.section .text
.global SchedSwitch

# void SchedSwitch(u32 *OldEsp, u32 NewEsp);
SchedSwitch:
    pushl %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    # save old task's stack pointer, then flip ESP to the new task's stack
    movl 20(%esp), %eax
    movl %esp, (%eax)
    movl 24(%esp), %esp

    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret