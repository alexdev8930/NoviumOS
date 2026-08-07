#include "drivers/novium/console.h"

static char *video_memory = (char *)0xB8000;
static int cursor = 0;

void console_write(const char *str)
{
    while (*str) {
        console_putchar(*str);
        str++;
    }
}

void console_putchar(char c) {
    video_memory[cursor * 2] = c;
    video_memory[cursor * 2 + 1] = 0x07;
    cursor++;
}

void console_clear(void) {
    for (cursor = 0; cursor < 2000; cursor++) {
        video_memory[cursor * 2] = ' ';
        video_memory[cursor * 2 + 1] = 0x07;
    }
    cursor = 0;
}