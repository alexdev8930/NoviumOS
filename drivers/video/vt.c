#include <novium/drivers/vt.h>

struct console_driver *con_driver = NULL;
bool console_line_echo = false;

int user_cmdline_start = 0;
static int con_cursor = 0;
static u32 con_fg = 0xAAAAAA;
static u32 con_bg = 0x000000;

static inline int con_cells(void)
{
	return con_driver ? (con_driver->cols * con_driver->rows) : 0;
}

static void sync_hw_cursor(void)
{
	if (con_driver && con_driver->set_cursor) {
		con_driver->set_cursor(con_cursor % con_driver->cols,
		                       con_cursor / con_driver->cols);
	}
}

static void con_scroll(void)
{
	int cols = con_driver->cols;
	int rows = con_driver->rows;

	if (con_driver->scroll)
		con_driver->scroll(1, con_fg, con_bg);
	else if (con_driver->clear)
		con_driver->clear(con_fg, con_bg);

	con_cursor = cols * (rows - 1);
	sync_hw_cursor();

	/* the prompt scrolled up with the rest of the screen */
	if (user_cmdline_start >= cols)
		user_cmdline_start -= cols;
	else
		user_cmdline_start = 0;
}

void console_register(struct console_driver *drv)
{
	con_driver = drv;
}

void console_init(struct console_driver *drv)
{
	con_driver = drv;
	con_cursor = 0;
	user_cmdline_start = 0;
	if (con_driver && con_driver->init)
		con_driver->init();
}

void console_putchar(char c)
{
	if (!con_driver || !con_driver->putc)
		return;

	int cols = con_driver->cols;
	int cells = con_cells();

	if (c == '\n') {
		int next = ((con_cursor / cols) + 1) * cols;
		if (next >= cells) {
			con_scroll();
		} else {
			con_cursor = next;
			sync_hw_cursor();
		}
		return;
	}

	if (c == '\t') {
		for (int i = 0; i < 4; i++)
			console_putchar(' ');
		return;
	}

	if (c == '\b') {
		if (con_cursor > user_cmdline_start) {
			con_cursor--;
			con_driver->putc(con_cursor % cols, con_cursor / cols, ' ', con_fg, con_bg);
			sync_hw_cursor();
		}
		return;
	}

	if (con_cursor >= cells)
		con_scroll();

	con_driver->putc(con_cursor % cols, con_cursor / cols, c, con_fg, con_bg);
	con_cursor++;

	if (con_cursor >= cells)
		con_scroll();
	else
		sync_hw_cursor();

	/* echoing a command line and the row just filled up, so mark the new one */
	if (console_line_echo && (con_cursor % cols) == 0)
		console_write("> ");
}

void console_write(const char *str)
{
	while (*str)
		console_putchar(*str++);
}

void console_clear(void)
{
	if (con_driver && con_driver->clear)
		con_driver->clear(con_fg, con_bg);

	con_cursor = 0;
	user_cmdline_start = 0;
	sync_hw_cursor();
}

void console_prompt(void)
{
	console_write("> ");
	user_cmdline_start = con_cursor;
}

int console_cursor(void)
{
	return con_cursor;
}

void console_set_cursor(int pos)
{
	int cells = con_cells();
	if (cells == 0)
		return;

	if (pos < 0)
		pos = 0;
	else if (pos >= cells)
		pos = cells - 1;

	con_cursor = pos;
	sync_hw_cursor();
}
