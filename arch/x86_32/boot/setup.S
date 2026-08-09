.code16
.section .text
.global setup_entry

setup_entry:
    # Print 'S' for starting setup phase
    movb $0x0E, %ah            
    movb $'S', %al
    int $0x10

    # --- FIX: Enable the A20 Line via BIOS ---
    movw $0x2401, %ax           # BIOS function: Enable A20
    int $0x15
    jnc a20_done                # If carry flag is clear, it succeeded!
    
    # Fast I/O fallback method if the BIOS call fails
    inb $0x92, %al
    orb $0x02, %al
    outb %al, $0x92

a20_done:
    # Load GDT descriptor
    lgdt (gdt_descriptor)
    
    # Flip the lowest bit in CR0 to enable protected mode
    movl %cr0, %eax
    orl $0x1, %eax             
    movl %eax, %cr0

    # Far jump clears the 16-bit cache and loads our 32-bit code segment
    ljmp $0x08, $protected_mode_start

.code32
protected_mode_start:
    # Point all data segments to our flat 4GB GDT entry (0x10)
    movw $0x10, %ax            
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x90000, %esp         # Temporary stack below 1MB mark

    # FIX: Corrected AT&T syntax for absolute address printing
    movw $0x0F50, 0xB8000     

    # Move kernel from low memory scratch space up past the 1MB wall
    movl $0x8600, %esi        
    movl $0x100000, %edi       
    
    # OPTIMIZATION: Increased copy size to 16,384 dwords (64KB total) 
    # to give your future Desktop/GUI kernel plenty of room to grow.
    movl $16384, %ecx            
    cld                        
    rep movsd
    
    jmp 0x100000               # Jump to our kernel entry point

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
