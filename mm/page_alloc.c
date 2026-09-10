#include <mm/page_alloc.h>

#define PAGE_COUNT 1048576u
#define BITMAP_WORDS (PAGE_COUNT / 32u)
#define MULTIBOOT_MEMORY_AVAILABLE 1u

extern char _kernel_start;
extern char _kernel_end;

static u32 PageBitmap[BITMAP_WORDS];
static u32 FreePages;

static void PageSet(u32 Page, u32 Free) {
	u32 Word = Page / 32u;
	u32 Bit = Page % 32u;

	if (Free != 0) {
		if ((PageBitmap[Word] & (1u << Bit)) == 0) {
			PageBitmap[Word] |= 1u << Bit;
			FreePages++;
		}
	} else if ((PageBitmap[Word] & (1u << Bit)) != 0) {
		PageBitmap[Word] &= ~(1u << Bit);
		FreePages--;
	}
}

/* marks a range of memory addresses as free or used, rounding to 4KB page boundaries. */
static void PageSetRange(u64 Base, u64 Length, u32 Free) {
	u64 End = Base + Length;
	u64 First;
	u64 Last;
	u32 Page;

	if (End < Base || Base >= 0x100000000ULL) {
		return;
	}

	if (End > 0x100000000ULL) {
		End = 0x100000000ULL;
	}

	First = (Base + PAGE_SIZE - 1u) & ~(u64)(PAGE_SIZE - 1u);
	Last = End & ~(u64)(PAGE_SIZE - 1u);

	for (; First < Last; First += PAGE_SIZE) {
		Page = (u32)(First / PAGE_SIZE);
		if (Page < PAGE_COUNT) {
			PageSet(Page, Free);
		}
	}
}

/* clears the bitmap, parses GRUB's memory map, and protects the kernel's memory space. */
void PageAllocInit(const struct boot_info *Boot) {
	u32 Index;
	u32 Offset;
	u32 EntrySize;
	struct multiboot_mmap *Entry;

	for (Index = 0; Index < BITMAP_WORDS; Index++) {
		PageBitmap[Index] = 0;
	}
	FreePages = 0;

	if (Boot == 0 || Boot->memory_map_addr == 0 || Boot->memory_map_length == 0) {
		return;
	}

	Offset = 0;
	while (Offset + sizeof(u32) <= Boot->memory_map_length) {
		Entry = (struct multiboot_mmap *)(Boot->memory_map_addr + Offset);
		EntrySize = Entry->size;

		if (EntrySize < 20u || EntrySize > Boot->memory_map_length - Offset - sizeof(u32)) {
			break;
		}

		if (Entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			PageSetRange(Entry->base_addr, Entry->length, 1);
		}

		Offset += sizeof(u32) + EntrySize;
	}

	PageSetRange(0, PAGE_SIZE, 0);
	PageSetRange((u32)&_kernel_start,
				 (u32)&_kernel_end - (u32)&_kernel_start, 0);
}

u32 PageAlloc(void) {
	u32 Word;
	u32 Bit;
	u32 Page;

	for (Word = 0; Word < BITMAP_WORDS; Word++) {
		if (PageBitmap[Word] == 0) {
			continue;
		}

		for (Bit = 0; Bit < 32u; Bit++) {
			if ((PageBitmap[Word] & (1u << Bit)) != 0) {
				Page = Word * 32u + Bit;
				PageSet(Page, 0);
				return Page * PAGE_SIZE;
			}
		}
	}

	return 0;
}

/* scans the bitmap for the first free 4KB page and returns its physical address. */
void PageFree(u32 Address) {
	if (Address == 0 || (Address & (PAGE_SIZE - 1u)) != 0) {
		return;
	}

	PageSet(Address / PAGE_SIZE, 1);
}

u32 PageAllocFreeCount(void) {
	return FreePages;
}
