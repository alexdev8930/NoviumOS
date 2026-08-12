#ifndef NOVIUM_PS2_H
#define NOVIUM_PS2_H

#include <asm/io.h>

#define PS2_DATA_PORT       0x60
#define PS2_STATUS_PORT     0x64
#define PS2_COMMAND_PORT    0x64

u8 ps2_read_status(void);
u8 ps2_read_scancode(void);

#endif
