.code16
.section .text
.global setup_entry

setup_entry:
   
    movb $0x0E, %ah            
    movb $'S', %al
    int $0x10

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

    # Print 'P' directly to VGA video memory text grid
    movw $0x0F50, (0xB8000)     

    # Move kernel from low memory scratch space up past the 1MB wall
    movl $0x8600, %esi        
    movl $0x100000, %edi       
    movl $4096, %ecx            # Copy 16KB (4096 dwords)
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
