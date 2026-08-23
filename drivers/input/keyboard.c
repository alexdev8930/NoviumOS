#include <kernel/irq.h>
#include <drivers/novium/input.h>
#include "ps2.h"

#define BUF_SIZE 32

// volatile forces the cpu to read from ram instead of caching in registers
static volatile u8 buf[BUF_SIZE];
static volatile u8 head = 0;
static volatile u8 tail = 0;

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