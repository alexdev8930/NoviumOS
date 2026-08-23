.section .text

# no error code stubs
.macro ISR_NOERR vector
.global isr_stub_\vector
isr_stub_\vector:
    pushl $0
    pushl $\vector
    jmp isr_common
.endm

# hardware error code stubs
.macro ISR_ERR vector
.global isr_stub_\vector
isr_stub_\vector:
    pushl $\vector      # if error code is already there just push vector      
    jmp isr_common
.endm

# shared interrupt handler
isr_common:
    pusha       # push all 8 registers, edi, esi, ebp, esp, ebx, edx, ecx, eax

    push %ds
    push %es
    push %fs
    push %gs

    # load kernel segment selector
    movw $0x10, %ax
    movw %ax, %ds  
    movw %ax, %es         
    movw %ax, %fs        
    movw %ax, %gs   

    cld         # clear direction flag so C code doesnt break

    # pass stack pointer to C handler as an argument
    pushl %esp
    call isr_dispatch
    addl $4, %esp

    # restore previous states
    pop %gs
    pop %fs             
    pop %es                
    pop %ds              
    popa

    # stack cleanup and return
    addl $8, %esp     
    iret

# CPU exceptions
ISR_NOERR 0                 # divide error
ISR_NOERR 1                 # debug exception
ISR_NOERR 2                 # nmi
ISR_NOERR 3                 # breakpoint
ISR_NOERR 4                 # overflow
ISR_NOERR 5                 # bound range exceeded
ISR_NOERR 6                 # invalid opcode
ISR_NOERR 7                 # device not available
ISR_ERR   8                 # double fault
ISR_NOERR 9                 # coprocessor segment overrun
ISR_ERR   10                # invalid tss
ISR_ERR   11                # segment not present
ISR_ERR   12                # stack fault
ISR_ERR   13                # general protection fault
ISR_ERR   14                # page fault
ISR_NOERR 15                # reserved
ISR_NOERR 16                # x87 fpu error
ISR_ERR   17                # alignment check
ISR_NOERR 18                # machine check
ISR_NOERR 19                # simd exception
ISR_NOERR 20                # virtualization exception
ISR_NOERR 21                # control protection
ISR_NOERR 22                # reserved
ISR_NOERR 23                # reserved
ISR_NOERR 24                # reserved
ISR_NOERR 25                # reserved
ISR_NOERR 26                # reserved
ISR_NOERR 27                # reserved
ISR_NOERR 28                # reserved
ISR_NOERR 29                # reserved
ISR_NOERR 30                # reserved
ISR_NOERR 31                # reserved

# hardware irqs remapped to 32-47
ISR_NOERR 32                # timer
ISR_NOERR 33                # keyboard
ISR_NOERR 34                # cascade
ISR_NOERR 35                # com2
ISR_NOERR 36                # com1
ISR_NOERR 37                # lpt2
ISR_NOERR 38                # floppy
ISR_NOERR 39                # lpt1 / spurious master
ISR_NOERR 40                # rtc clock
ISR_NOERR 41                # acpi
ISR_NOERR 42                # pci unassigned
ISR_NOERR 43                # pci unassigned
ISR_NOERR 44                # mouse
ISR_NOERR 45                # fpu error
ISR_NOERR 46                # primary ata
ISR_NOERR 47                # secondary ata / spurious slave
