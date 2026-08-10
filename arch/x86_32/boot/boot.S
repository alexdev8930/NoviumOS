.code16
.section .text
.global _start

_start:
    cli                         # Early setup stack and segment sanitization
    xorw %ax, %ax               
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp           

    movb %dl, %bl               # Save active BIOS boot drive ID
    sti                         

    movb $0x0E, %ah            
    movb $'L', %al
    int $0x10
    
    movw $0x0000, %ax          # Target low memory buffer address 0x7E00
    movw %ax, %es
    movw $0x7E00, %bx
    
    movb $0x02, %ah            # Read setup segment (4 sectors)
    movb $4, %al               
    movb $0, %ch               
    movb $2, %cl               
    movb $0, %dh               
    movb %bl, %dl              
    int $0x13
    jc disk_error              
    
    movw $0x8600, %bx          # Advance buffer to kernel chunk 1

    movb $0x02, %ah            # Read chunk 1 to finish Cylinder 0 Head 0 track
    movb $13, %al              
    movb $0, %ch               
    movb $6, %cl               
    movb $0, %dh               
    movb %bl, %dl              
    int $0x13
    jc disk_error                

    movw $0xA000, %bx          # Advance buffer to kernel chunk 2

    movb $0x02, %ah            # Read chunk 2 on Cylinder 0 Head 1 track
    movb $18, %al              
    movb $0, %ch               
    movb $1, %cl               
    movb $1, %dh               
    movb %bl, %dl              
    int $0x13
    jc disk_error                

    movw $0xC400, %bx          # Advance buffer to final sector destination

    movb $0x02, %ah            # Read final remaining sector on Cylinder 1 Head 0
    movb $1, %al               
    movb $1, %ch               
    movb $1, %cl               
    movb $0, %dh               
    movb %bl, %dl              
    int $0x13
    jnc boot_ok

disk_error:
    movb $0x0E, %ah            # Output error indicator and freeze execution
    movb $'E', %al
    int $0x10
    cli
_hang:
    hlt
    jmp _hang

boot_ok:
    movb %bl, %dl               # Pass drive parameter and hand execution to setup.S
    ljmp $0x0000, $0x7E00      

.org 510                       
.word 0xAA55                   # Boot signature marker
