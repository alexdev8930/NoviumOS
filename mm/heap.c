#include "page_alloc.h"
#include <novium/heap.h>

#define HEAP_MAGIC 0x48454150u /* ASCII "HEAP" */

typedef struct HeapBlock {
    u32 Magic;
    u32 PageCount;
    u32 Address;
} HeapBlock;

/* allocate a single kernel memory page */
void *kmalloc(size_t size) {    
    if (size == 0 || size > PAGE_SIZE - sizeof(HeapBlock)) {
        return 0;
    }

    u32 Address = PageAlloc();

    if (Address == 0) {
        return 0;
    }

    HeapBlock *Block = (HeapBlock *)Address;

    Block->Magic = HEAP_MAGIC;
    Block->PageCount = 1;
    Block->Address = Address;

    return (void *)(Block + 1);
}

/* free a single kernel memory page */
void kfree(void *addr) {
    if (addr == 0) {
        return;
    }

    HeapBlock *Block = ((HeapBlock *)addr) - 1;

    if (Block->Magic != HEAP_MAGIC || Block->PageCount != 1) {
        return;
    }

    u32 PageAddress = Block->Address;

    Block->Magic = 0;
    Block->PageCount = 0;
    Block->Address = 0;

    PageFree(PageAddress);
}