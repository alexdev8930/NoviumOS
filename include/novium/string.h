#ifndef NOVIUM_STRING_H
#define NOVIUM_STRING_H

#include "types.h"

void *memset(void *s, int c, size_t len);
void *memmove(void *dest, const void *src, size_t len);
void *memcpy(void *dest, const void *src, size_t len);
size_t strlen(const char *str); 
int strcmp(const char *s1, const char *s2);

#endif


