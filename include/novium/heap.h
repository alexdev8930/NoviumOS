#ifndef NOVIUM_HEAP_H
#define NOVIUM_HEAP_H

#include "types.h"

void HeapInit(void);
void *kmalloc(size_t size);
void kfree(void *addr);

#endif