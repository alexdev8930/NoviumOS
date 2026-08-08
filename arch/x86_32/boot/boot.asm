section .multiboot
align 4
    dd 0x1BADB002             
    dd 0x00000003              
    dd -(0x1BADB002 + 0x00000003)

section .text
global _start
extern setup_entry              ; declared in setup.asm

_start:
    cli                        
    mov esp, temp_stack_top     
    jmp setup_entry             ; jump straight to setup.asm

section .bss
align 16
temp_stack_bottom:
    resb 4096                   ; 4 KB temporary stack
temp_stack_top:
