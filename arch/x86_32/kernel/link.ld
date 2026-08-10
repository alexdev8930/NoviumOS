ENTRY(bootstrap_entry)

SECTIONS
{
    . = 1M;                     /* Base memory location counter */

    .text : ALIGN(4096) 
    {
        KEEP(*(.text.prologue)) /* Lock entry code to front */
        *(.text*)
    }

    .rodata : ALIGN(4096) 
    { 
        *(.rodata*) 
    }
    
    .data : ALIGN(4096) 
    { 
        *(.data*) 
    }
    
    .bss : ALIGN(4096) 
    { 
        _bss_start = .;       /* Start symbol for zero-out routine */
        *(COMMON) 
        *(.bss*) 
        _bss_end = .;         /* End symbol for zero-out routine */
    }
}
