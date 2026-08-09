.section .multiboot
.align 4
.long 0x1BADB002                 # Multiboot Magic Number flags
.long 0x00000003                 # Align modules + memory payload information info
.long -(0x1BADB002 + 0x00000003) # Valid Checksum signature

.section .text
.global bootstrap_entry

bootstrap_entry:
    cli                         # Guarantee interrupts are masked
    
    # Initialize proper execution stack tracking
    movl $stack_top, %esp       
    
    # Clear direction flag for subsequent C array/string mechanics
    cld                         

    # Chain directly to your early C engine
    .extern hw_init
    call hw_init

_halt:
    hlt
    jmp _halt

.section .bss
.align 16
stack_bottom:
    .skip 4096                  # 4KB Kernel Execution Stack space
stack_top:
