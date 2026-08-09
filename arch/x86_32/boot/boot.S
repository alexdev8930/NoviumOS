.code16
.section .text
.global _start

_start:
    cli                         # Disable interrupts during segment setup
    xorw %ax, %ax               
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp           # Stack grows down from 0x7C00 away from code

    movb %dl, %bl               # Save boot drive ID safely
    sti                         # Re-enable interrupts

    # Print 'L' to show loading phase started
    movb $0x0E, %ah            
    movb $'L', %al
    int $0x10
    
    # Set destination buffer address to 0x0000:0x7E00
    movw $0x0000, %ax          
    movw %ax, %es
    movw $0x7E00, %bx
    
    # Read 36 sectors from the disk
    movb $0x02, %ah            
    movb $36, %al              # 4 setup sectors + 32 kernel sectors
    movb $0, %ch               # Cylinder 0
    movb $2, %cl               # Sector 2 (immediately after MBR)
    movb $0, %dh               # Head 0
    movb %bl, %dl              # Restore boot drive ID
    int $0x13
    jnc boot_ok                
    
    # Print 'E' and halt if disk read fails
    movb $0x0E, %ah
    movb $'E', %al
    int $0x10
    cli
_hang:
    hlt
    jmp _hang

boot_ok:
    movb %bl, %dl               # Pass drive ID to setup.S
    ljmp $0x0000, $0x7E00      # Far jump to the next stage

.org 510                       
.word 0xAA55                   # MBR Boot Signature
