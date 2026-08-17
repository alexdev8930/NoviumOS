#include "ps2.h"

u8 ps2_read_status(void) {
    return inb(PS2_STATUS_PORT);
}

u8 ps2_read_scancode(void) {
    return inb(PS2_DATA_PORT);
}