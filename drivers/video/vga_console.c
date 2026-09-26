#include <drivers/console.h>
#include <asm/io.h>
#include <novium/string.h>
#include <novium/stdio.h>

#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25
#define CONSOLE_CELLS  (CONSOLE_WIDTH * CONSOLE_HEIGHT)

static char *video_memory = (char *)0xB8000;
static int cursor = 0;
int user_cmdline_start = 0;
int console_line_echo = 0;  

/* the only place that moves the cursor, so it can never leave the text page */
static void console_set_cursor(int position) {
    if (position < 0) {
        position = 0;
    } else if (position >= CONSOLE_CELLS) {
        position = CONSOLE_CELLS - 1;
    }

    cursor = position;
    update_hardware_cursor(cursor);
}

static void console_scroll(void) {
    size_t copy_size = CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1) * 2;
    memmove((void*)video_memory, (void*)(video_memory + (CONSOLE_WIDTH * 2)), copy_size);

    char *bottom_row = video_memory + copy_size;
    for (int i = 0; i < CONSOLE_WIDTH; i++) {
        bottom_row[i * 2] = ' ';
        bottom_row[i * 2 + 1] = 0x07;
    }

    console_set_cursor(CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1));

    /* the prompt scrolled up with the rest of the screen */
    if (user_cmdline_start >= CONSOLE_WIDTH) {
        user_cmdline_start -= CONSOLE_WIDTH;
    } else {
        user_cmdline_start = 0;
    }
}

void console_putchar(char c) {
    if (c == '\n') {
        int next = ((cursor / CONSOLE_WIDTH) + 1) * CONSOLE_WIDTH;

        if (next >= CONSOLE_CELLS) {
            console_scroll();
        } else {
            console_set_cursor(next);
        }
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
            console_set_cursor(cursor);
        }
        return;
    }

    video_memory[cursor * 2] = c;
    video_memory[cursor * 2 + 1] = 0x07;

    if (cursor + 1 >= CONSOLE_CELLS) {
        console_scroll();
    } else {
        console_set_cursor(cursor + 1);
    }

    /* echoing a command line and the row just filled up, so mark the new one */
    if (console_line_echo != 0 && (cursor % CONSOLE_WIDTH) == 0) {
        console_write("> ");
    }
}


void console_prompt(void) {
    kprintf("> ");
    user_cmdline_start = cursor;
}

void console_write(const char *str) {
    while (*str) {
        console_putchar(*str++);
    }
}

void console_clear(void) {
    memset(video_memory, 0x20, CONSOLE_WIDTH * CONSOLE_HEIGHT * 2);
    for (int i = 0; i < CONSOLE_WIDTH * CONSOLE_HEIGHT; i++) {
        video_memory[i * 2 + 1] = 0x07;
    }

    /* GRUB may leave the VGA text cursor disabled. */
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0E);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);

    console_set_cursor(0);
    user_cmdline_start = 0;
}


void update_hardware_cursor(int pos) {
    /* select low cursor byte register (0x0F) on port 0x3D4, write value to 0x3D5 */
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    /* select high cursor byte register (0x0E) on port 0x3D4, write value to 0x3D5 */
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)((pos >> 8) & 0xFF));
}
