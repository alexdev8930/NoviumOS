ENTRY(bootstrap_entry)

SECTIONS
{
    . = 1M; 

    .text : ALIGN(4K) 
    {
        *(.multiboot)
        build/bootstrap.o(.text)
        *(.text)
    }

    .rodata : ALIGN(4K) { *(.rodata*) }
    .data   : ALIGN(4K) { *(.data) }
    .bss    : ALIGN(4K) { *(COMMON) *(.bss) }
}
