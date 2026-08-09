.code16
.section .text
.global _start

_start:
    cli                         
    xorw %ax, %ax               # Set up flat real mode segments
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp           # Temporary stack pointer below the MBR

    movb %dl, %dh               # Save the BIOS boot drive number
    sti

    # Print 'L' for loading
    movb $0x0E, %ah            
    movb $'L', %al
    int $0x10
    
    # Target address to load setup.S stage (0x0000:0x7E00)
    movw $0x0000, %ax          
    movw %ax, %es
    movw $0x7E00, %bx
    
    # Read setup block and kernel together
    movb $0x02, %ah            
    movb $36, %al              # 4 sectors for setup + 32 sectors for kernel
    movb $0, %ch               
    movb $2, %cl               # Start right after MBR sector
    movb $0, %dh               
    movb %dh, %dl              # Restore our saved drive ID
    int $0x13
    jnc boot_ok                
    
    # Print 'E' if disk read fails
    movb $0x0E, %ah
    movb $'E', %al
    int $0x10
    cli
    hlt

boot_ok:
    movb %dh, %dl              # Pass drive number to the next stage
    ljmp $0x0000, $0x7E00      # Jump to setup.S entry point

.org 510                       
.word 0xAA55                   # Mandatory MBR signature
