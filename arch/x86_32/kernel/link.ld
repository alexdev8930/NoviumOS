ENTRY(bootstrap_entry)

SECTIONS
{
    . = 1M;
    _kernel_start = .;

    .multiboot : ALIGN(4)
    {
        KEEP(*(.multiboot))
    }

    . = ALIGN(4096);
    .text :
    {
        KEEP(*(.text.prologue))
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

    . = ALIGN(4096);
    _kernel_end = .;
}