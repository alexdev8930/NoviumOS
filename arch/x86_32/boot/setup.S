.code16
.section .text
.global setup_entry

setup_entry:
    # Debug: Print 'S' (Setup Active)
    movb $0x0E, %ah            
    movb $'S', %al
    int $0x10

    # Load GDT (Calculate absolute descriptor base manually from 0x7E00)
    lgdt (gdt_descriptor)
    
    # Toggle Protected Mode Bit in CR0
    movl %cr0, %eax
    orl $0x1, %eax             
    movl %eax, %cr0

    # 16-bit to 32-bit far jump flushes prefetch pipeline queue
    ljmp $0x08, $protected_mode_start

.code32
protected_mode_start:
    # Synchronize data segments with flat descriptor index 0x10
    movw $0x10, %ax            
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x90000, %esp         # Set up temporary stack base

    # Debug: Print 'P' directly to top-left of VGA text memory mapping
    movw $0x0F50, (0xB8000)     

    # Relocate kernel: copy payload from scratch space (0x8600) up to 1MB
    movl $0x8600, %esi        
    movl $0x100000, %edi       
    movl $4096, %ecx            # 16KB / 4 = 4096 copies
    cld                        
    rep movsd
    
    jmp 0x100000               # Jump straight into linked bootstrap.S at 1MB

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
