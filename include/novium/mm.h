#ifndef NOVIUM_MM_H
#define NOVIUM_MM_H

#include <mm/page_alloc.h>
#include <mm/paging.h>
#include <novium/types.h>

void *kmalloc(size_t size);
void kfree(void *address);

#endif
