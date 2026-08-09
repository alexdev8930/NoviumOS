.code16
.section .text
.global setup_entry

setup_entry:
    # Print 'S' to signal setup stage
    movb $0x0E, %ah            
    movb $'S', %al
    int $0x10

    # Enable A20 via BIOS
    movw $0x2401, %ax           
    int $0x15
    jnc a20_done                
    
    # Fallback to Fast A20 gate if BIOS fails
    inb $0x92, %al
    orb $0x02, %al
    outb %al, $0x92

a20_done:
    lgdt (gdt_descriptor)
    
    # Turn on protected mode bit in CR0
    movl %cr0, %eax
    orl $0x1, %eax             
    movl %eax, %cr0

    # Far jump to flush 16-bit cache and load 32-bit CS
    ljmp $0x08, $protected_mode_start

.code32
protected_mode_start:
    # Set up 32-bit flat data segments (0x10 selector)
    movw $0x10, %ax            
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x90000, %esp         # Temporary stack below 1MB

    # Write a white 'P' to VGA text memory (Fixed absolute address syntax)
    movw $0x0F50, (0xB8000)     

    # Copy kernel payload from low memory scratch space to the 1MB mark
    movl $0x8600, %esi        
    movl $0x100000, %edi       
    movl $16384, %ecx            
    cld                        
    rep movsd
    
    jmp 0x100000               # Jump to bootstrap_entry at 1MB

.align 4
gdt_start:
    .long 0x0, 0x0             
gdt_code:
    .long 0x0000FFFF, 0x00CF9A00 
gdt_data:
    .long 0x0000FFFF, 0x00CF9200 
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1 
    .long gdt_start
