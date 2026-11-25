#ifndef NOVIUM_I8042_H
#define NOVIUM_I8042_H

#include <novium/types.h>

#define I8042_DATA_PORT     0x60
#define I8042_STATUS_PORT   0x64
#define I8042_COMMAND_PORT  0x64

u8 i8042_read_status(void);
u8 i8042_read_scancode(void);

#endif
