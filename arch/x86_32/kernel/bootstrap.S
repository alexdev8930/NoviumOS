.section .text.prologue
.global bootstrap_entry

bootstrap_entry:
    cli

    # save GRUB values before using EAX for segment setup.
    movl %eax, %edx
    movl %ebx, %esi

    movw $0x18, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp
    cld

    # clear BSS.
    movl $_bss_start, %edi
    movl $_bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    # c calling convention: push arguments right-to-left.
    pushl %esi
    pushl %edx
    call hw_init

_halt:
    hlt
    jmp _halt

.section .bss
.align 16
stack_bottom:
    .skip 16384                 
stack_top:
