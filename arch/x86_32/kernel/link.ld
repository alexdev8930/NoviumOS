ENTRY(bootstrap_entry)

SECTIONS
{
    . = 1M;                     /* load kernel at 1mb */

    .text : ALIGN(4096) 
    {
        KEEP(*(.text.prologue)) /* make sure entry code comes first */
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
        _bss_start = .;       
        *(COMMON) 
        *(.bss*) 
        _bss_end = .;         
    }
}
