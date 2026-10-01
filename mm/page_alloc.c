#include <mm/page_alloc.h>

#define PAGE_COUNT 1048576u
#define BITMAP_WORDS (PAGE_COUNT / 32u)

/* Bit to write for a page in the bitmap: 1 is free, 0 is used. */
typedef enum { PageStateUsed = 0, PageStateAvailable = 1 } PageState;

extern char _kernel_start;
extern char _kernel_end;

static u32 PageBitmap[BITMAP_WORDS];
static u32 FreePages;

static void PageSet(u32 Page, PageState State) {
	u32 Word = Page / 32u;
	u32 Bit = Page % 32u;

	if (State == PageStateAvailable) {
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
static void PageSetRange(u64 Base, u64 Length, PageState State) {
	u64 End = Base + Length;
	u64 First, Last;

	if (End < Base || Base >= 0x100000000ULL) {
		return;
	}
	if (End > 0x100000000ULL) {
		End = 0x100000000ULL;
	}

	if (State == PageStateAvailable) {
		First = (Base + PAGE_SIZE - 1u) & ~(u64)(PAGE_SIZE - 1u);
		Last = End & ~(u64)(PAGE_SIZE - 1u);
	} else {
		First = Base & ~(u64)(PAGE_SIZE - 1u);
		Last = (End + PAGE_SIZE - 1u) & ~(u64)(PAGE_SIZE - 1u);
	}

	for (; First < Last; First += PAGE_SIZE) {
		u32 Page = (u32)(First / PAGE_SIZE);
		if (Page < PAGE_COUNT) {
			PageSet(Page, State);
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

		if (Entry->type == MultibootMemoryAvailable) {
			PageSetRange(Entry->base_addr, Entry->length, PageStateAvailable);
		}

		Offset += sizeof(u32) + EntrySize;
	}

	PageSetRange(0, 0x100000, PageStateUsed);
	PageSetRange((u32)&_kernel_start, (u32)&_kernel_end - (u32)&_kernel_start, PageStateUsed);
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
				PageSet(Page, PageStateUsed);
				return Page * PAGE_SIZE;
			}
		}
	}

	return 0;
}

u32 PageAllocPages(u32 count) {
	u32 Start;
	u32 Run;
	u32 Page;

	if (count == 0 || count > FreePages || count > PAGE_COUNT) {
		return 0;
	}

	for (Start = 0; Start <= PAGE_COUNT - count; Start++) {
		for (Run = 0; Run < count; Run++) {
			Page = Start + Run;

			if ((PageBitmap[Page / 32u] & (1u << (Page % 32u))) == 0) {
				break;
			}
		}

		if (Run == count) {
			for (Run = 0; Run < count; Run++) {
				PageSet(Start + Run, PageStateUsed);
			}

			return Start * PAGE_SIZE;
		}

		/* Skip past the first used page found, no valid run can begin before that page. */
		Start += Run;
	}
	return 0;
}

/* scans the bitmap for the first free 4KB page and returns its physical address. */
void PageFree(u32 Address) {
	if (Address == 0 || (Address & (PAGE_SIZE - 1u)) != 0) {
		return;
	}

	if (Address / PAGE_SIZE >= PAGE_COUNT) {
		return;
	}

	PageSet(Address / PAGE_SIZE, PageStateAvailable);
}

void PageFreePages(u32 Address, u32 Count) {
	u32 Start;
	u32 Index;

	if (Address == 0 || (Address & (PAGE_SIZE - 1u)) != 0 || Count == 0) {
		return;
	}

	Start = Address / PAGE_SIZE;

	if (Start >= PAGE_COUNT || Count > PAGE_COUNT - Start) {
		return;
	}

	for (Index = 0; Index < Count; Index++) {
		PageSet(Start + Index, PageStateAvailable);
	}
}

u32 PageAllocFreeCount(void) {
	return FreePages;
}
