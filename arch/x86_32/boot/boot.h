#ifndef NOVIUM_X86_32_BOOT_H
#define NOVIUM_X86_32_BOOT_H


#define MBR_LOAD_ADDR           0x7C00u  
#define SETUP_STAGE_LOAD_ADDR   0x7E00u  
#define KERNEL_TEMP_BUFFER      0x8600u  
#define KERNEL_CHUNK2_ADDR      0xA000u    
#define KERNEL_CHUNK3_ADDR      0xC400u      
#define KERNEL_LOAD_ADDR        0x100000u 
#define INITIAL_STACK_TOP       0x90000u  
#define BOOT_DRIVE_ADDR         0x5000u   


#define SETUP_SECTOR_COUNT      4         
#define KERNEL_CHUNK1_SECTORS   13        
#define KERNEL_CHUNK2_SECTORS   18        
#define KERNEL_CHUNK3_SECTORS    1      
#define KERNEL_TOTAL_SECTORS    (KERNEL_CHUNK1_SECTORS + KERNEL_CHUNK2_SECTORS \
                                 + KERNEL_CHUNK3_SECTORS)  

#endif 
