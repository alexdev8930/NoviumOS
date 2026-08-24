#include "boot.h"

.code16
.section .text
.global _start

_start:
    # base system configuration and boot sign of life
    cli                         
    xorw %ax, %ax                    
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $MBR_LOAD_ADDR, %sp         
    movb %dl, %bl              
    sti                               

    movb $0x0E, %ah           
    movb $'L', %al
    int $0x10          

    # pull setup stage into memory directly past the bootloader
    movw $0x0000, %ax          
    movw %ax, %es
    movw $SETUP_STAGE_LOAD_ADDR, %bx          
    movb $0x02, %ah            
    movb $SETUP_SECTOR_COUNT, %al                    
    movb $0, %ch                    
    movb $2, %cl                    
    movb $0, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jc disk_error                        

    # load kernel chunk 1 
    movw $KERNEL_TEMP_BUFFER, %bx          
    movb $0x02, %ah            
    movb $KERNEL_CHUNK1_SECTORS, %al                   
    movb $0, %ch                    
    movb $6, %cl                    
    movb $0, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jc disk_error                      

    # load kernel chunk 2 
    movw $KERNEL_CHUNK2_ADDR, %bx          
    movb $0x02, %ah            
    movb $KERNEL_CHUNK2_SECTORS, %al                   
    movb $0, %ch                    
    movb $1, %cl                    
    movb $1, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jc disk_error                      

    # load final sector
    movw $KERNEL_CHUNK3_ADDR, %bx          
    movb $0x02, %ah            
    movb $KERNEL_CHUNK3_SECTORS, %al                    
    movb $1, %ch                    
    movb $1, %cl                    
    movb $0, %dh                    
    movb %bl, %dl                   
    int $0x13     
    jnc boot_ok  

disk_error:
    # print error and trap execution loop
    movb $0x0E, %ah            
    movb $'E', %al
    int $0x10
    cli
_hang:
    hlt
    jmp _hang  

boot_ok:
    movb %bl, %dl               # pass drive ID to setup
    ljmp $0x0000, $SETUP_STAGE_LOAD_ADDR        

.org 510                        
.word 0xAA55                   
