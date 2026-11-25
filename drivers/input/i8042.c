#include <asm/io.h>
#include <novium/drivers/i8042.h>

u8 i8042_read_status(void) {
    return inb(I8042_STATUS_PORT);
}

u8 i8042_read_scancode(void) {
    while (!(i8042_read_status() & 0x01)) {
    }
    return inb(I8042_DATA_PORT);
}
