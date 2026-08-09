.code16
.section .text
.global _start

_start:
    cli                         # Disable interrupts during segment setup
    xorw %ax, %ax               
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp           # Set up temporary stack below MBR

    movb %dl, %bl               # Save boot drive ID away from INT 0x13
    sti                         # Re-enable interrupts

   
    movb $0x0E, %ah            
    movb $'L', %al
    int $0x10
    
 
    movw $0x0000, %ax          
    movw %ax, %es
    movw $0x7E00, %bx
    
 
    movb $0x02, %ah            
    movb $36, %al              # Read 36 sectors (4 setup + 32 kernel)
    movb $0, %ch               # Cylinder 0
    movb $2, %cl               # Sector 2 (right after MBR)
    movb $0, %dh               # Head 0
    movb %bl, %dl              # Restore saved drive ID
    int $0x13
    jnc boot_ok                
    

    movb $0x0E, %ah
    movb $'E', %al
    int $0x10
    cli
    hlt

boot_ok:
    movb %bl, %dl               # Pass drive ID to next stage
    ljmp $0x0000, $0x7E00      # Jump to setup.S


.org 510                       
.word 0xAA55                   # Mandatory MBR signature
