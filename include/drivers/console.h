#ifndef NOVIUM_CONSOLE_H
#define NOVIUM_CONSOLE_H

#include <novium/types.h>

void console_write(const char *str);
void console_putchar(char c);
void console_clear(void);
void console_prompt(void);
void update_hardware_cursor(int pos);

/* Cursor position, in cells. */
extern int user_cmdline_start;

/* True while a command line is echoed, so the prompt is reprinted on wrap. */
extern bool console_line_echo;

#endif