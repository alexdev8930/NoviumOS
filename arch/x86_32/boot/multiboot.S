#include "multiboot.h"
.section .multiboot, "a"
.align 8

.global multiboot_header
multiboot_header:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT2_ARCHITECTURE_I386
    .long multiboot_header_end - multiboot_header
    .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))

    /* Framebuffer tag (type 5, optional, size 20) */
    .align 8
    .word MULTIBOOT2_HEADER_TAG_FRAMEBUFFER
    .word MULTIBOOT2_HEADER_TAG_OPTIONAL
    .long 20
    .long 800   /* width */
    .long 600   /* height */
    .long 32    /* depth (bpp) */

    /* End tag (type 0, flags 0, size 8) */
    .align 8
    .word MULTIBOOT2_HEADER_TAG_END
    .word 0
    .long 8

multiboot_header_end:
