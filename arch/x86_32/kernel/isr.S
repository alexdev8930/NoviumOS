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
    pusha       # push edi, esi, ebp, esp, ebx, edx, ecx, eax

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

# CPU Exceptions (0-31) 
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

# Hardware IRQs (32-47) 
ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47
