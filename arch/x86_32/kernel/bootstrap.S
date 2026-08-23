.section .text.prologue
.global bootstrap_entry

bootstrap_entry:
    cli                        

    # reload data segments
    movw $0x10, %ax             
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movl $stack_top, %esp       # set up stack pointer
    cld                         

    # clear bss section
    .extern _bss_start
    .extern _bss_end
    movl $_bss_start, %edi      
    movl $_bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb                   

    # jump to c code
    .extern hw_init
    call hw_init                

_halt:
    hlt                         
    jmp _halt

.section .bss
.align 16
stack_bottom:
    .skip 16384                 
stack_top:
