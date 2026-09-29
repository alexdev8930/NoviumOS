#include "page_alloc.h"
#include <novium/heap.h>

#define HEAP_MAGIC 0x48454150u   /* ASCII "HEAP", catches a bad pointer */
#define HEAP_GROW_PAGES 16
#define HEAP_ALIGN 8
#define HEAP_ALIGN_UP(x) (((x) + (HEAP_ALIGN - 1)) & ~(u32)(HEAP_ALIGN - 1))


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
static Block *HeapEnd;   

/* Put a block on the free list, keeping it sorted by address. */
static void HeapListInsert(Block *b) {
    Block *cur = FreeListHead;
    Block *prev = 0;

    while (cur != 0 && cur < b) {
        prev = cur;
        cur = cur->Node.Next;
    }
    b->Node.Prev = prev;
    b->Node.Next = cur;

    if (prev) {
        prev->Node.Next = b;
    } else {
        FreeListHead = b;
    }
    if (cur) {
        cur->Node.Prev = b;
    }
}

/* Take a block off the free list. Works for head, middle, or only block. */
static void HeapListRemove(Block *b) {
    if (b->Node.Prev) {
        b->Node.Prev->Node.Next = b->Node.Next;
    } else {
        FreeListHead = b->Node.Next;
    }

    if (b->Node.Next) {
        b->Node.Next->Node.Prev = b->Node.Prev;
    }

    b->Node.Prev = 0;
    b->Node.Next = 0;
}

/* Find a free block of at least Size bytes. */
static Block *HeapFindFree(u32 Size) {
    Block *b = FreeListHead;

    while (b != 0) {
        if (b->Meta.Size >= Size) {
            HeapListRemove(b);
            return b;
        }
        b = b->Node.Next;
    }

    return 0;
}

static Block *HeapGrow(u32 pages) {
    u32 addr = PageAllocPages(pages);
    if (addr == 0) return 0;

    /* Only allowed to extend the same run */
    if (HeapEnd != 0 && (u32)HeapEnd != addr) {
        PageFreePages(addr, pages);      /* Hand it straight back */
        return 0;
    }

    Block *b = (Block *)addr;
    b->Meta.Magic    = HEAP_MAGIC;
    b->Meta.Size     = pages * PAGE_SIZE;
    b->Meta.PrevSize = 0;
    b->Meta.IsFree   = true;

    HeapListInsert(b);
    HeapEnd = (Block *)(addr + pages * PAGE_SIZE);
    return b;
}

/* Cut an oversized block in two. */
static void HeapSplit(Block *b, u32 Size) {
    u32 LeftOver = b->Meta.Size - Size;
    Block *next;
    Block *rest;

    if (LeftOver < HEAP_ALIGN + sizeof(BlockMeta)) {
        return;
    }

    rest = (Block *)((u8 *)b + Size);
    rest->Meta.Magic    = HEAP_MAGIC;
    rest->Meta.Size     = LeftOver;
    rest->Meta.PrevSize = Size;
    rest->Meta.IsFree   = true;

    next = (Block *)((u8 *)rest + LeftOver);     /* From rest, not b */
    if (next < HeapEnd) {
        next->Meta.PrevSize = LeftOver;
    }

    b->Meta.Size = Size;      /* Shrink last */
    HeapListInsert(rest);
}

void HeapInit(void) {
    FreeListHead = 0;
    HeapGrow(HEAP_GROW_PAGES);
}

/* WORK IN PROGRESS */
void *kmalloc(size_t size) {
    if (size == 0) {
        return 0;
    }
    if (size > (u32)-1 - sizeof(BlockMeta) - HEAP_ALIGN) {
        return 0;
    }

    u32 TotalSize = HEAP_ALIGN_UP(size + sizeof(BlockMeta));
    u32 OriginalSize;
    Block *next;
    Block *b = HeapFindFree(TotalSize);

    if (b == 0) {
        u32 pages = (TotalSize + PAGE_SIZE - 1) / PAGE_SIZE;
        b = HeapGrow(pages);
        if (b == 0) {
            return 0;
        }
        HeapListRemove(b);   /* HeapGrow left it on the list */
    }

    OriginalSize = b->Meta.Size;   /* The real size, before we touch it */

    if (OriginalSize > TotalSize) {
        HeapSplit(b, TotalSize);
    } else {
        b->Meta.Size = TotalSize;
    }

    b->Meta.Magic  = HEAP_MAGIC;
    b->Meta.IsFree = false;

    next = (Block *)((u8 *)b + b->Meta.Size);
    if (next < HeapEnd) {
        next->Meta.PrevSize = b->Meta.Size;
    }

    return (void *)((u8 *)b + sizeof(BlockMeta));
}

/* WORK IN PROGRESS */
void kfree(void *addr) {
    Block *b = (Block *)((u8 *)addr - sizeof(BlockMeta));
    (void)b;
}
