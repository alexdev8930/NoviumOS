.code16
.section .text
.global _start

_start:
    cli                         # Disable interrupts during configuration
    xorw %ax, %ax               # Clear data segment selectors
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp           # Set stack execution baseline securely

    movb %dl, %dh               # Cache the boot drive index in dh
    sti

    # Debug: Print 'L' (Loading)
    movb $0x0E, %ah            
    movb $'L', %al
    int $0x10
    
    # Load destination segment memory base: 0x0000:0x7E00
    movw $0x0000, %ax          
    movw %ax, %es
    movw $0x7E00, %bx
    
    # Read setup block and kernel off the physical disk tracks
    movb $0x02, %ah            # BIOS sector read command
    movb $36, %al              # Read 36 sectors total (4 Setup + 32 Kernel)
    movb $0, %ch               # Cylinder 0
    movb $2, %cl               # Start reading at Sector 2
    movb $0, %dh               # Head 0
    movb %dh, %dl              # Restore the drive ID from dh
    int $0x13
    jnc boot_ok                # Jump if carry flag is zero
    
    # Debug: Print 'E' (Error) if disk reading fails
    movb $0x0E, %ah
    movb $'E', %al
    int $0x10
    cli
    hlt

boot_ok:
    movb %dh, %dl              # Restore drive number to dl register
    ljmp $0x0000, $0x7E00      # Absolute far jump straight to setup.S load base

.org 510                       # Enforce signature placement at byte 510
.word 0xAA55                   # MBR structural verification token
