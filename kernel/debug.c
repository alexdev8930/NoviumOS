#include <novium/debug.h>
#include <drivers/console.h>

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

