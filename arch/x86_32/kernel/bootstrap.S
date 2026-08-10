.section .text.prologue
.global bootstrap_entry

bootstrap_entry:
    cli                         # Halt maskable interrupts

    movw $0x10, %ax             # Sync segment registers with GDT data selector
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp       # Transition to execution stack allocation pointer
    cld                         

    .extern _bss_start
    .extern _bss_end
    movl $_bss_start, %edi      # Baseline address mapping for uninitialized blocks
    movl $_bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb                   # Flash memory range blocks down to zero state

    .extern hw_init
    call hw_init                # Transition execution thread directly into C logic

_halt:
    hlt                         # Catch termination drops and park processor
    jmp _halt

.section .bss
.align 16
stack_bottom:
    .skip 16384                 
stack_top:
