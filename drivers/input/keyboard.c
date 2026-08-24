#include <kernel/irq.h>
#include <drivers/novium/input.h>
#include "ps2.h"

#define BUF_SIZE 32

// we use volatile because interrupts modify these in RAM
static volatile u8 buf[BUF_SIZE];
static volatile u8 head = 0;
static volatile u8 tail = 0;


static const char kbd_layout[] = {
    [0x01] = 27,   
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', 
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8', 
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=', 
    [0x0E] = '\b', 
    [0x0F] = '\t', 
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', 
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', 
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']', 
    [0x1C] = '\n',  
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', 
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', 
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`', 
    [0x2B] = '\\', 
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', 
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',', 
    [0x34] = '.', [0x35] = '/', 
    [0x39] = ' '   
};

static const char kbd_layout_shift[] = {
    [0x01] = 27,   
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', 
    [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*', 
    [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+', 
    [0x0E] = '\b', 
    [0x0F] = '\t', 
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', 
    [0x14] = 'T', [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', 
    [0x18] = 'O', [0x19] = 'P', [0x1A] = '{', [0x1B] = '}', 
    [0x1C] = '\n', 
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', 
    [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', 
    [0x26] = 'L', [0x27] = ':', [0x28] = '"', [0x29] = '~', 
    [0x2B] = '|', 
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', 
    [0x30] = 'B', [0x31] = 'N', [0x32] = 'M', [0x33] = '<', 
    [0x34] = '>', [0x35] = '?', 
    [0x39] = ' '
};

static int shift_held = 0;

static void keyboard_callback(struct registers *r) {
    (void)r;

    u8 sc = ps2_read_scancode();

    // drop the keystroke if the queue is completely full
    u8 next = (head + 1) % BUF_SIZE;
    if (next != tail) {
        buf[head] = sc;
        head = next;
    }
}

void keyboard_init(void) {
    irq_register(1,  keyboard_callback);
}

int keyboard_pop(void) {
    if (tail == head) {
        return -1;
    }
    int sc = buf[tail];
    tail = (tail + 1) % BUF_SIZE;
    return sc;
}

// decode the scancodes
int keyboard_getchar(void) {
    for (;;) {
        int sc = keyboard_pop();
        if (sc < 0) {
            asm volatile("hlt"); 
            continue;
        }
        if (sc & 0x80) {
            sc &= 0x7F;
            if (sc == 0x2A || sc == 0x36) {
                shift_held = 0;
            }
            continue;
        }
        if (sc == 0x2A || sc == 0x36) {
            shift_held = 1;
            continue;
        }

        if (sc == 0x3A) {
            shift_held = !shift_held;
            continue;
        }

        const char *layout = shift_held ? kbd_layout_shift : kbd_layout;
        if (sc >= (int)sizeof(kbd_layout)) {
            continue;
        }

        char key = layout[sc];
        if (key == 0) {
            continue;
        }
        return key;
    }
}