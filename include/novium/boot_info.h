#ifndef NOVIUM_BOOT_INFO_H
#define NOVIUM_BOOT_INFO_H

#include <novium/types.h>

#define BOOT_INFO_MAGIC         0x4E4F5601u  

struct boot_info {
    u32 magic;            
    u32 boot_drive_id;
    u32 memory_size_kb;    
    u32 reserved[5];
};

#endif 
