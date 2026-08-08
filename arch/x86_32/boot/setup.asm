section .text

global setup_entry
extern bootstrap_entry

setup_entry:
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1               ; enable protected mode
    mov cr0, eax

    jmp 0x08:complete_flush

bits 32
complete_flush:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, stack_top
    jmp bootstrap_entry        ; jump to the GNU assembly bridge

section .bss
align 16
stack_space:
    resb 4096
stack_top:

section .rodata
align 4
gdt_start:
    dd 0x0, 0x0               ; null descriptor
gdt_code:
    dd 0x0000FFFF, 0x00CF9A00 ; 32-bit code segment descriptor
gdt_data:
    dd 0x0000FFFF, 0x00CF9200 ; 32-bit data segment descriptor
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; GDT size
    dd gdt_start              ; GDT address