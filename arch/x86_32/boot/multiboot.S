#include "multiboot.h"
.section .multiboot, "a"
.align 4

.global multiboot_header
multiboot_header:
    .long MULTIBOOT_HEADER_MAGIC
    .long MULTIBOOT_HEADER_FLAGS
    .long MULTIBOOT_HEADER_CHECKSUM