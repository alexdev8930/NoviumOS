#ifndef NOVIUM_X86_32_BOOT_H
#define NOVIUM_X86_32_BOOT_H


#define MBR_LOAD_ADDR           0x7C00u   // Where BIOS drops boot.S 
#define SETUP_STAGE_LOAD_ADDR   0x7E00u   // Where setup.S executes  
#define KERNEL_TEMP_BUFFER      0x8600u   // Disk scratch buffer, chunk 1 base 
#define KERNEL_CHUNK2_ADDR      0xA000u   // 0x8600 + 13 sectors        
#define KERNEL_CHUNK3_ADDR      0xC400u   // 0xA000 + 18 sectors        
#define KERNEL_LOAD_ADDR        0x100000u // Permanent home of the kernel 
#define INITIAL_STACK_TOP       0x90000u  // Temporary bootstrap stack top  
#define BOOT_DRIVE_ADDR         0x5000u   // Handoff byte: BIOS boot drive (%dl) from real mode


#define SETUP_SECTOR_COUNT      4         // bytes: 2K (per Makefile truncate) 
#define KERNEL_CHUNK1_SECTORS   13        // 1664 dwords -> $0x8600..0xA000 
#define KERNEL_CHUNK2_SECTORS   18        // 2304 dwords -> $0xA000..0xC400 
#define KERNEL_CHUNK3_SECTORS    1        // 128 dwords  -> $0xC400..0xC600 
#define KERNEL_TOTAL_SECTORS    (KERNEL_CHUNK1_SECTORS + KERNEL_CHUNK2_SECTORS \
                                 + KERNEL_CHUNK3_SECTORS)  

#endif 
