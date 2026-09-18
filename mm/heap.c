#include "page_alloc.h"
#include <novium/heap.h>

#define HEAP_MAGIC 0x48454150u /* ASCII "HEAP" */

typedef struct HeapBlock {
    u32 Magic;
    u32 PageCount;
    u32 Address;
} HeapBlock;

/* allocate one or more contiguous kernel memory pages */
void *kmalloc(size_t size) {    
    size_t TotalSize;
    u32 pages;

    if (size == 0) {
        return 0;
    }

    if (size > (size_t)-1 - sizeof(HeapBlock) - (PAGE_SIZE - 1u)) {
        return 0;
    }

    TotalSize = size + sizeof(HeapBlock) + PAGE_SIZE - 1u;
    pages = (u32)(TotalSize / PAGE_SIZE);

    u32 Address = PageAllocPages(pages);

    if (Address == 0) {
        return 0;
    }

    HeapBlock *Block = (HeapBlock *)Address;

    Block->Magic = HEAP_MAGIC;
    Block->PageCount = pages;
    Block->Address = Address;

    return (void *)(Block + 1);
}

/* free a contiguous kernel memory allocation */
void kfree(void *addr) {
    if (addr == 0) {
        return;
    }

    HeapBlock *Block = ((HeapBlock *)addr) - 1;

    if (Block->Magic != HEAP_MAGIC || Block->PageCount == 0) {
        return;
    }

    u32 Address = Block->Address;
    u32 PageCount = Block->PageCount;

    Block->Magic = 0;
    Block->PageCount = 0;
    Block->Address = 0;

    PageFreePages(Address, PageCount);
}