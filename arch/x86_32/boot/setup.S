.code16
.section .text
.global setup_entry

setup_entry:
    movb $0x0E, %ah            # Print status character
    movb $'S', %al
    int $0x10

    movw $0x2401, %ax          # Route A20 line enable through BIOS
    int $0x15
    jnc a20_done                
    
    inb $0x92, %al             # Fallback to fast A20 gate switch
    orb $0x02, %al
    outb %al, $0x92

a20_done:
    lgdt (gdt_descriptor)      # Load staging global descriptor table
    
    movl %cr0, %eax            # Toggle execution tracking mode bit
    orl $0x1, %eax             
    movl %eax, %cr0

    ljmp $0x08, $protected_mode_start # Flush pipeline and step to 32-bit execution

.code32
protected_mode_start:
    movw $0x10, %ax            # Synchronize selector layout tracking registers
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x90000, %esp        

    movw $0x0F50, (0xB8000)    # Direct absolute screen verification write

    movl $0x8600, %esi         # Stage 1: Load payload segment 1 to 1MB mark
    movl $0x100000, %edi       
    movl $1664, %ecx            
    cld                        
    rep movsd

    movl $0xA000, %esi         # Stage 2: Link sequential payload segment 2
    movl $2304, %ecx            
    rep movsd

    movl $0xC400, %esi         # Stage 3: Link payload tail termination sector
    movl $128, %ecx             
    rep movsd
    
    jmp 0x100000               # Transfer focus straight to bootstrap_entry

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
