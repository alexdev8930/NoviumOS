#include <drivers/console.h>
#include <asm/io.h>

static char *video_memory = (char *)0xB8000;
static int cursor = 0;

void console_putchar(char c) {
    if (c == '\n') {
        cursor = ((cursor / 80) + 1) * 80;
        update_hardware_cursor(cursor);
        return;
    }

    if (c == '\t') {
        for (int i = 0; i < 4; i++) {
            console_putchar(' '); 
        }
        return;
    }

    if (c == '\b') {
        if (cursor > 0) { 
            cursor--; 
            video_memory[cursor * 2] = ' '; 
            video_memory[cursor * 2 + 1] = 0x07; 
            update_hardware_cursor(cursor); 
        }
        return;
    }

    video_memory[cursor * 2] = c;
    video_memory[cursor * 2 + 1] = 0x07;
    cursor++;
    
    // update the flashing cursor position for normal characters as well
    update_hardware_cursor(cursor);
}

void console_write(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void console_clear(void) {
    for (int i = 0; i < 2000; i++) {
        video_memory[i * 2] = ' ';
        video_memory[i * 2 + 1] = 0x07;
    }
    cursor = 0;
    update_hardware_cursor(cursor);
}

void update_hardware_cursor(int pos) {
    // select low cursor byte register (0x0F) on port 0x3D4, write value to 0x3D5
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    // select high cursor byte register (0x0E) on port 0x3D4, write value to 0x3D5
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}
