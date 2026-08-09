ENTRY(bootstrap_entry)

SECTIONS
{
    /* Link the kernel strictly at the 1 Megabyte wall */
    . = 1M; 

    /* Group all execution code together */
    .text : ALIGN(4096) 
    {
        /* The linker will look for bootstrap_entry and force it to be the first byte */
        KEEP(*(.text.prologue)) 
        *(.text*)
    }

    /* Standard read-only data, global variables, and uninitialized stacks */
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
        _bss_start = .;       /* Mark the beginning of uninitialized variables */
        *(COMMON) 
        *(.bss*) 
        _bss_end = .;         /* Mark the end */
    }
}
