#ifndef NOVIUM_CONSOLE_H
#define NOVIUM_CONSOLE_H

void console_write(const char *str);
void console_putchar(char c);
void console_clear(void);
void update_hardware_cursor(int pos);

#endif