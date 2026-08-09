.section .multiboot
.align 4
.long 0x1BADB002                 # Magic number required by multiboot spec
.long 0x00000003                 # Boot flags (page align modules + memory info)
.long -(0x1BADB002 + 0x00000003) # Checksum to prove header validity

.section .text
.global bootstrap_entry

bootstrap_entry:
    cli                         # Keep interrupts disabled during initialization
    
    movl $stack_top, %esp       # Initialize our real 32-bit kernel stack
    cld                         # Clear direction flag for string copy safety

    .extern hw_init
    call hw_init

_halt:
    hlt
    jmp _halt

.section .bss
.align 16
stack_bottom:
    .skip 4096                  # 4KB kernel stack space
stack_top:
