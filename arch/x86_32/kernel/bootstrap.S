.global bootstrap_entry
.extern hw_init

.section .text
.code32

bootstrap_entry:
    /* set up the 32-bit stack pointer before hitting C */
    movl $stack_top, %esp

    call hw_init

_halt:
    cli
    hlt
    jmp _halt

/* allocate physical stack space in the BSS section */
.section .bss
.balign 8
stack_bottom:
    .skip 16384               # reserves 16 KB of memory for C functions
stack_top:
