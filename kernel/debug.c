#include <kernel/debug.h>
#include <drivers/novium/console.h>

static const char hex_digits[] = "0123456789ABCDEF";


void debug_print_hex8(u8 v) {
    console_putchar(hex_digits[v >> 4]);
    console_putchar(hex_digits[v & 0xF]);
}

void debug_print_hex32(u32 v) {
    for (int i = 28; i >= 0; i -= 4) {
        console_putchar(hex_digits[(v >> i) & 0xF]);
    }
}


void debug_print_dec(u32 v) {
    char buf[11];               
    int i = 10;
    buf[i] = '\0';
    do {
        buf[--i] = '0' + (v % 10);
        v /= 10;
    } while (v != 0);
    console_write(&buf[i]);    
}
