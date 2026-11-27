#include "multiboot.h"
.section .multiboot, "a"
.align 4

.global multiboot_header
multiboot_header:
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long MULTIBOOT_HEADER_CHECKSUM

    .long 0 /* header_addr */
    .long 0 /* load_addr */
    .long 0 /* load_end_addr */
    .long 0 /* bss_end_addr */
    .long 0 /* entry_addr */

    .long 0   /* mode_type: 0 = linear graphics */
    .long 800 /* width */
    .long 600 /* height */
    .long 32  /* depth */
