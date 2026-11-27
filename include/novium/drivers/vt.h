#ifndef NOVIUM_VT_H
#define NOVIUM_VT_H

#include <novium/boot_info.h>
#include <novium/types.h>

/* Display backend hooks (vgacon, fbcon, etc). vt.c handles cursor
 * movement and line state, then calls into these to draw cells. */
struct console_driver {
	int cols;
	int rows;
	void (*init)(void);
	void (*putc)(int x, int y, char c, u32 fg, u32 bg);
	void (*clear)(u32 fg, u32 bg);
	void (*scroll)(int lines, u32 fg, u32 bg);
	void (*set_cursor)(int x, int y);
};

extern struct console_driver *con_driver;
extern bool console_line_echo;
extern int user_cmdline_start;

void console_register(struct console_driver *drv);
void console_init(struct console_driver *drv);
void console_putchar(char c);
void console_write(const char *str);
void console_clear(void);
void console_prompt(void);
int console_cursor(void);
void console_set_cursor(int pos);

bool vgacon_init(void);
bool fbcon_init(struct boot_info *boot);

#endif
