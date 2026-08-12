#ifndef NOVIUM_IO_H
#define NOVIUM_IO_H

#include <novium/types.h>


static inline u8 inb(u16 port) {
    u8 result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static inline void outb(u16 port, u8 data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline void io_wait() {
    outb(0x80, 0x00);
}

#endif