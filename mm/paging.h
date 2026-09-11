 #ifndef NOVIUM_PAGING_H
 #define NOVIUM_PAGING_H

 #include <novium/types.h>

 #define PAGING_PAGE_SIZE 4096u
 #define PAGING_PRESENT   0x001u
 #define PAGING_WRITABLE  0x002u
 #define PAGING_USER      0x004u

 void PagingInit(void);
 void PagingMap(u32 VirtualAddress, u32 PhysicalAddress, u32 Flags);
 void PagingUnmap(u32 VirtualAddress);
 u32 PagingIsEnabled(void);

 #endif
/* TODO: System stub - implement when ready. */
