#ifndef NOVIUM_PAGE_ALLOC_H
#define NOVIUM_PAGE_ALLOC_H

#include <novium/boot_info.h>
#include <novium/types.h>

#define PAGE_SIZE 4096u

void PageAllocInit(const struct boot_info *Boot);
u32 PageAlloc(void);
void PageFree(u32 Address);
u32 PageAllocFreeCount(void);

#endif
/* TODO: System stub - implement when ready. */
