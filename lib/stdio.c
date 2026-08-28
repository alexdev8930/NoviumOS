#include <novium/stdio.h>
#include <novium/types.h>
#include <drivers/console.h>
#include <stdarg.h>

static void print_u32(u32 value, u32 base, const char *digits) {
    char buf[16];
    int i = 0;

    do {
        buf[i++] = digits[value % base];
        value /= base;
    } while (value > 0);

    while (i > 0) {
        console_putchar(buf[--i]);
    }
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (; *fmt != '\0'; fmt++) {
        if (*fmt != '%') {
            console_putchar(*fmt); 
            continue;
        }

        fmt++;
        switch (*fmt){
            case 'c':
                console_putchar((char)va_arg(args, int));
                break;
            case 's': {
                const char *str = va_arg(args, const char *);
                console_write(str);
                break;
            }
            case 'd':
            case 'i': {
                s32 val = va_arg(args, s32);
                if (val < 0) {
                    console_putchar('-');
                    val = -val;
                }
                print_u32((u32)val, 10, "0123456789");
                break;
            }
            case 'u':
                print_u32(va_arg(args, u32), 10, "0123456789");
                break;
            case 'x':
                print_u32(va_arg(args, u32), 16, "0123456789abcdef");
                break;
            case 'X':
                print_u32(va_arg(args, u32), 16, "0123456789ABCDEF");
                break;
            case '%':
                console_putchar('%');
                break;
            default:
                console_putchar(*fmt);
                break;
        }       
    }
    
    va_end(args);
    return 0;
}