.section .multiboot, "a"
.align 4

.global multiboot_header
multiboot_header:
    .long 0x1BADB002
    .long 0x00000003
    .long 0xE4524FFB