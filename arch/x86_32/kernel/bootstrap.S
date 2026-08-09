.section .text.prologue
.global bootstrap_entry

bootstrap_entry:
    cli                         # Disable interrupts during early setup
    
    # Reload segment registers with the 32-bit GDT data selector (0x10)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp       # Set up our initial kernel stack pointer
    cld                         # Ensure string operations increment forward

    # Clear the BSS section so uninitialized global C variables start at 0
    .extern _bss_start
    .extern _bss_end
    movl $_bss_start, %edi
    movl $_bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    .extern hw_init
    call hw_init                # Hand execution over to our C hardware initialization

_halt:
    hlt                         # Safety net: freeze the CPU if the kernel returns
    jmp _halt

.section .bss
.align 16
stack_bottom:
    .skip 16384                 # Allocate 16KB of stack space
stack_top:
