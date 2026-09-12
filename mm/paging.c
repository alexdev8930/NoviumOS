#include <mm/paging.h>

#define PAGE_TABLE_ENTRIES 1024u
#define PAGE_DIRECTORY_FLAGS (PAGING_PRESENT | PAGING_WRITABLE)

static u32 PageDirectory[PAGE_TABLE_ENTRIES]
	__attribute__((aligned(PAGING_PAGE_SIZE)));
static u32 PageTables[PAGE_TABLE_ENTRIES][PAGE_TABLE_ENTRIES]
	__attribute__((aligned(PAGING_PAGE_SIZE)));
static u32 PagingEnabled;

static u32 PageDirectoryIndex(u32 Address) {
	return Address >> 22;
}

static u32 PageTableIndex(u32 Address) {
	return (Address >> 12) & 0x3ffu;
}

/* maps a virtual address to a physical address with custom flags */
void PagingMap(u32 VirtualAddress, u32 PhysicalAddress, u32 Flags) {
	u32 DirectoryIndex;
	u32 TableIndex;

	VirtualAddress &= ~(PAGING_PAGE_SIZE - 1u);
	PhysicalAddress &= ~(PAGING_PAGE_SIZE - 1u);

	DirectoryIndex = PageDirectoryIndex(VirtualAddress);
	TableIndex = PageTableIndex(VirtualAddress);

	PageDirectory[DirectoryIndex] =
		((u32)&PageTables[DirectoryIndex][0] & ~(PAGING_PAGE_SIZE - 1u)) |
		PAGE_DIRECTORY_FLAGS;
	PageTables[DirectoryIndex][TableIndex] = PhysicalAddress | Flags;

	__asm__ __volatile__("invlpg (%0)" : : "r"(VirtualAddress) : "memory");
}

/* removes an existing virtual page mapping */
void PagingUnmap(u32 VirtualAddress) {
	u32 DirectoryIndex;
	u32 TableIndex;

	VirtualAddress &= ~(PAGING_PAGE_SIZE - 1u);
	DirectoryIndex = PageDirectoryIndex(VirtualAddress);
	TableIndex = PageTableIndex(VirtualAddress);

	PageTables[DirectoryIndex][TableIndex] = 0;
	__asm__ __volatile__("invlpg (%0)" : : "r"(VirtualAddress) : "memory");
}

/* set up basic identity maps and enable paging via CR0 and CR3 registers */
void PagingInit(void) {
	u32 DirectoryIndex;
	u32 TableIndex;
	u32 Address;

	for (DirectoryIndex = 0; DirectoryIndex < PAGE_TABLE_ENTRIES; DirectoryIndex++) {
		PageDirectory[DirectoryIndex] =
			((u32)&PageTables[DirectoryIndex][0] & ~(PAGING_PAGE_SIZE - 1u)) |
			PAGE_DIRECTORY_FLAGS;

		for (TableIndex = 0; TableIndex < PAGE_TABLE_ENTRIES; TableIndex++) {
			Address = (DirectoryIndex << 22) | (TableIndex << 12);
			PageTables[DirectoryIndex][TableIndex] =
				Address | PAGE_DIRECTORY_FLAGS;
		}
	}

	__asm__ __volatile__("movl %0, %%cr3" : : "r"((u32)PageDirectory) : "memory");

	__asm__ __volatile__(
		"movl %%cr0, %%eax\n"
		"orl $0x80000000, %%eax\n"
		"movl %%eax, %%cr0\n"
		:
		:
		: "eax", "memory");

	PagingEnabled = 1;
}

u32 PagingIsEnabled(void) {
	return PagingEnabled;
}
