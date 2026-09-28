#include "page_alloc.h"
#include <novium/heap.h>

#define HEAP_MAGIC 0x48454150u   /* ASCII "HEAP", catches a bad pointer */
#define HEAP_GROW_PAGES 16

typedef struct Block Block;

typedef struct BlockMeta {
    u32 Magic;
    u32 Size;
    u32 PrevSize;
    bool IsFree;
} BlockMeta;

typedef struct FreeNode {
    Block *Prev;
    Block *Next;
} FreeNode;

struct Block {
    BlockMeta Meta;
    union {
        FreeNode Node;
        u8 Payload[0]; /* GCC extension: zero bytes, just marks where data starts */
    };
};

static Block *FreeListHead;

void HeapInit(void) {
    FreeListHead = 0;
    u32 addr = PageAllocPages(HEAP_GROW_PAGES);
    if (addr == 0) {
        return;
    }

    Block *b = (Block *)addr;
    b->Meta.Magic = HEAP_MAGIC;
    b->Meta.Size = HEAP_GROW_PAGES * PAGE_SIZE;
    b->Meta.PrevSize = 0;
    b->Meta.IsFree = true;
    b->Node.Prev = 0;
    b->Node.Next = 0;

    FreeListHead = b;
}

/* WORK IN PROGRESS */
void *kmalloc(size_t size) {
    u32 TotalSize = size + sizeof(BlockMeta);

    Block *prev = 0;
    Block *b = FreeListHead;

    while (b != 0) {
        if (b->Meta.Size >= TotalSize) break;
        prev = b;
        b = b->Node.Next;
    }

    if (b == 0) {
        return 0; /* Genuinely out of memory */
    }


    if (b != 0) {
        if (prev) {
            prev->Node.Next = b->Node.Next;     
        } else {
            FreeListHead = b->Node.Next;       
        }
        if (b->Node.Next) {
            b->Node.Next->Node.Prev = prev; 
        }
    }

    b->Meta.Magic    = HEAP_MAGIC;
    b->Meta.Size     = TotalSize;
    b->Meta.PrevSize = 0;
    b->Meta.IsFree   = false;

    return (void *)((u8 *)b + sizeof(BlockMeta));
}

/* WORK IN PROGRESS */
void kfree(void *addr) {
    Block *b = (Block *)((u8 *)addr - sizeof(BlockMeta));
    (void)b;
}
