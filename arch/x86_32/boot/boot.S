.code16
.section .text
.global _start

_start:
    cli                         # disable interrupts for setup
    xorw %ax, %ax                    
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp           # setup stack below bootloader
    movb %dl, %bl               # save bios drive id
    sti                               

    movb $0x0E, %ah             # print 'L' to show we started
    movb $'L', %al
    int $0x10          

    # load setup segment (4 sectors) to 0x7E00
    movw $0x0000, %ax          
    movw %ax, %es
    movw $0x7E00, %bx          
    movb $0x02, %ah            
    movb $4, %al                    
    movb $0, %ch                    
    movb $2, %cl                    
    movb $0, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jc disk_error                        

    # load kernel chunk 1 (13 sectors)
    movw $0x8600, %bx          
    movb $0x02, %ah            
    movb $13, %al                   
    movb $0, %ch                    
    movb $6, %cl                    
    movb $0, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jc disk_error                      

    # load kernel chunk 2 (18 sectors)
    movw $0xA000, %bx          
    movb $0x02, %ah            
    movb $18, %al                   
    movb $0, %ch                    
    movb $1, %cl                    
    movb $1, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jc disk_error                      

    # load final sector
    movw $0xC400, %bx          
    movb $0x02, %ah            
    movb $1, %al                    
    movb $1, %ch                    
    movb $1, %cl                    
    movb $0, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jnc boot_ok  

disk_error:
    movb $0x0E, %ah            
    movb $'E', %al
    int $0x10
    cli
_hang:
    hlt
    jmp _hang  

boot_ok:
    movb %bl, %dl               # pass drive ID to setup
    ljmp $0x0000, $0x7E00        

.org 510                        
.word 0xAA55                   
