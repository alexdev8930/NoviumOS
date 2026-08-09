.section .text.prologue
.global bootstrap_entry

bootstrap_entry:
    cli                         # Turn off interrupts until we finish setting up
    movl $stack_top, %esp       # Point our CPU to the top of our new kernel stack
    cld                         # Make sure string operations move forward in memory

    .extern hw_init
    call hw_init                # Jump over to our C hardware setup code

_halt:
    hlt                         # If the kernel somehow exits, freeze the CPU
    jmp _halt                   # Infinite loop fallback just to be safe

.section .bss
.align 16
stack_bottom:
    .skip 16384                 # Give ourselves 16KB of stack space for desktop/GUI apps
stack_top:
