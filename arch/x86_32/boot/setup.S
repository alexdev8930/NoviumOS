.code16
.section .text
.global _start

_start:
    movb $0x0E, %ah            # print 'S' for setup stage
    movb $'S', %al
    int $0x10

    # enable A20 using bios
    movw $0x2401, %ax          
    int $0x15
    jnc a20_done                
    
    # fast A20 fallback if bios fails
    inb $0x92, %al             
    orb $0x02, %al
    outb %al, $0x92

a20_done:
    data32 lgdt (gdt_descriptor)    
    
    # switch to protected mode 
    movl %cr0, %eax            
    orl $0x1, %eax             
    movl %eax, %cr0

   # clear prefetch queue and flush cs to 32-bit segment
    ljmp $0x08, $protected_mode_start 

.code32
protected_mode_start:
    # reload segment registers
    movw $0x10, %ax            
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x90000, %esp        

    # print 'P' to top left of screen via VGA memory to verify pm
    movw $0x0F50, (0xB8000)    

    # move kernel chunk 1 to 1MB
    movl $0x8600, %esi         
    movl $0x100000, %edi       
    movl $1664, %ecx            
    cld                        
    rep movsl

    # append kernel chunk 2
    movl $0xA000, %esi         
    movl $2304, %ecx            
    rep movsl

    # append final kernel sector
    movl $0xC400, %esi         
    movl $128, %ecx             
    rep movsl
    
    # jump to kernel entry point at 1MB
    jmp 0x100000               

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
