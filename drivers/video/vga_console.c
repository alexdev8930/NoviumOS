#include <drivers/console.h>
#include <asm/io.h>
#include <novium/string.h>
#include <novium/stdio.h>

static char *video_memory = (char *)0xB8000;
static int cursor = 0;
int user_cmdline_start = 0;

static void console_scroll(void) {
    size_t copy_size = 80 * 24 * 2;
    memmove((void*)video_memory, (void*)(video_memory + (80 * 2)), copy_size);
    
    char *bottom_row = video_memory + copy_size;
    for (int i = 0; i < 80; i++) {
        bottom_row[i * 2] = ' ';
        bottom_row[i * 2 + 1] = 0x07;
    }
    cursor = 80 * 24;
}

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
        if (cursor > user_cmdline_start) {  
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
    
    if (cursor >= 80 * 25) {
        console_scroll();
    }

    update_hardware_cursor(cursor);
}


void console_prompt(void) {
    printf("> ");
    user_cmdline_start = cursor;
}

void console_write(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void console_clear(void) {
    memset(video_memory, 0x20, 80 * 25 * 2);       
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i * 2 + 1] = 0x07;             
    }
    cursor = 0; user_cmdline_start = 0;
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
