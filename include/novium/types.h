#ifndef NOVIUM_TYPES_H
#define NOVIUM_TYPES_H

/* Freestanding, so bool/true/false work without a libc. */
#include <stdbool.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef unsigned long size_t;
typedef long ssize_t;

#define NULL ((void*)0)

#endif