ENTRY(bootstrap_entry)

SECTIONS
{
    /* Link the kernel strictly at the 1 Megabyte wall */
    . = 1M; 

    /* Group all execution code together */
    .text : ALIGN(4) 
    {
        /* The linker will look for bootstrap_entry and force it to be the first byte */
        KEEP(*(.text.prologue)) 
        *(.text*)
    }

    /* Standard read-only data, global variables, and uninitialized stacks */
    .rodata : ALIGN(4) { *(.rodata*) }
    .data   : ALIGN(4) { *(.data*) }
    .bss    : ALIGN(4) { *(COMMON) *(.bss*) }
}
