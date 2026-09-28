#include <drivers/console.h>
#include <asm/io.h>
#include <novium/string.h>
#include <novium/stdio.h>

/* Text page geometry: 80x25 cells of two bytes each, at 0xB8000. */
enum {
    ConsoleWidth = 80,
    ConsoleHeight = 25,
    ConsoleCells = ConsoleWidth * ConsoleHeight
};

/*
 * VGA text mode palette; an attribute byte is background << 4 | foreground.
 *
 * Packed, because a plain enum is int sized here: four bytes of palette in a
 * text cell would pad it and shift every cell after it by two bytes.
 */
typedef enum __attribute__((packed)) {
    VgaBlack = 0,
    VgaBlue,
    VgaGreen,
    VgaCyan,
    VgaRed,
    VgaMagenta,
    VgaBrown,
    VgaLightGrey,
    VgaDarkGrey,
    VgaLightBlue,
    VgaLightGreen,
    VgaLightCyan,
    VgaLightRed,
    VgaLightMagenta,
    VgaYellow,
    VgaWhite
} VgaColor;

/* Packed picks the smallest type that fits, so an enumerator past 255 would
 * silently widen the palette again. Fail the build instead. */
_Static_assert(sizeof(VgaColor) == 1,
               "VgaColor grew past a byte, text cells will misalign");

#define VGA_ATTRIBUTE(Background, Foreground) \
    ((u8)((Background) << 4) | (u8)(Foreground))

/* Light grey on black: what the ROM leaves text mode set to. */
enum { ConsoleAttribute = VGA_ATTRIBUTE(VgaBlack, VgaLightGrey) };

static char *video_memory = (char *)0xB8000;
static int cursor = 0;
int user_cmdline_start = 0;
bool console_line_echo = false;  

/* the only place that moves the cursor, so it can never leave the text page */
static void console_set_cursor(int position) {
    if (position < 0) {
        position = 0;
    } else if (position >= ConsoleCells) {
        position = ConsoleCells - 1;
    }

    cursor = position;
    update_hardware_cursor(cursor);
}

static void console_scroll(void) {
    size_t copy_size = ConsoleWidth * (ConsoleHeight - 1) * 2;
    memmove((void*)video_memory, (void*)(video_memory + (ConsoleWidth * 2)), copy_size);

    char *bottom_row = video_memory + copy_size;
    for (int i = 0; i < ConsoleWidth; i++) {
        bottom_row[i * 2] = ' ';
        bottom_row[i * 2 + 1] = ConsoleAttribute;
    }

    console_set_cursor(ConsoleWidth * (ConsoleHeight - 1));

    /* the prompt scrolled up with the rest of the screen */
    if (user_cmdline_start >= ConsoleWidth) {
        user_cmdline_start -= ConsoleWidth;
    } else {
        user_cmdline_start = 0;
    }
}

void console_putchar(char c) {
    if (c == '\n') {
        int next = ((cursor / ConsoleWidth) + 1) * ConsoleWidth;

        if (next >= ConsoleCells) {
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
            video_memory[cursor * 2 + 1] = ConsoleAttribute;
            console_set_cursor(cursor);
        }
        return;
    }

    video_memory[cursor * 2] = c;
    video_memory[cursor * 2 + 1] = ConsoleAttribute;

    if (cursor + 1 >= ConsoleCells) {
        console_scroll();
    } else {
        console_set_cursor(cursor + 1);
    }

    /* echoing a command line and the row just filled up, so mark the new one */
    if (console_line_echo && (cursor % ConsoleWidth) == 0) {
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
    memset(video_memory, 0x20, ConsoleWidth * ConsoleHeight * 2);
    for (int i = 0; i < ConsoleWidth * ConsoleHeight; i++) {
        video_memory[i * 2 + 1] = ConsoleAttribute;
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
