ENTRY(_start)

SECTIONS
{
    . = 1M; /* tell bootloader to load OS in 1 MB in ram */
    

    /* intructions */
    .text : ALIGN(4K) 
    {
        *(.multiboot)
        *(.text)
    }


    /* read only data section */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    /* data section */
    .data : ALIGN(4K)
    {
        *(.data)
    }

    /* bss section */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
}