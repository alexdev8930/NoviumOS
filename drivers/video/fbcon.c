#include "font8x16.h"
#include <novium/boot_info.h>
#include <novium/cpu.h>
#include <novium/drivers/vt.h>
#include <novium/sched.h>
#include <novium/string.h>
#include <novium/timer.h>

enum { FontWidth = 8, FontHeight = 16, CursorStackSize = 4096 };

/* Framebuffer state filled in by fbcon_init from multiboot boot_info. */
static volatile u32 *fb_addr;
static u32 fb_pitch;
static u32 fb_width;
static u32 fb_height;
static u32 fb_bpp;

static int cursor_x = 0;
static int cursor_y = 0;
static bool cursor_on = true;
static bool blink_started = false;
static u8 cursor_stack[CursorStackSize] __attribute__((aligned(16)));
static u32 last_key_tick = 0;

static inline void fb_put_pixel(int x, int y, u32 color) {
	if (x < 0 || x >= (int)fb_width || y < 0 || y >= (int)fb_height) {
		return;
	}

	volatile u8 *p = (volatile u8 *)fb_addr + (u32)y * fb_pitch + (u32)x * (fb_bpp / 8);
	if (fb_bpp == 32) {
		*(volatile u32 *)p = color;
	} else if (fb_bpp == 24) {
		*(p + 0) = (u8)(color & 0xFF);
		*(p + 1) = (u8)((color >> 8) & 0xFF);
		*(p + 2) = (u8)((color >> 16) & 0xFF);
	}
}

static void draw_cursor_bar(int cx, int cy, u32 color) {
	int px = cx * FontWidth;
	int py = cy * FontHeight + (FontHeight - 2);

	for (int row = 0; row < 2; row++) {
		for (int col = 0; col < FontWidth; col++) {
			fb_put_pixel(px + col, py + row, color);
		}
	}
}

static void fbcon_putc(int x, int y, char c, u32 fg, u32 bg) {
	int px = x * FontWidth;
	int py = y * FontHeight;
	const u8 *glyph = &font8x16[(u8)c * FontHeight];

	for (int row = 0; row < FontHeight; row++) {
		u8 bits = glyph[row];
		for (int col = 0; col < FontWidth; col++) {
			u32 color = (bits & (0x80 >> col)) ? fg : bg;
			fb_put_pixel(px + col, py + row, color);
		}
	}
}

static void fbcon_clear(u32 fg, u32 bg) {
	(void)fg;
	for (int y = 0; y < (int)fb_height; y++) {
		for (int x = 0; x < (int)fb_width; x++) {
			fb_put_pixel(x, y, bg);
		}
	}

	cursor_x = 0;
	cursor_y = 0;
	cursor_on = true;
}

static void fbcon_scroll(int lines, u32 fg, u32 bg) {
	if (lines <= 0) {
		return;
	}

	u32 text_rows = fb_height / FontHeight;
	if ((u32)lines >= text_rows) {
		fbcon_clear(fg, bg);
		return;
	}

	/* erase the cursor bar before shifting pixels so it doesn't leave a trail */
	draw_cursor_bar(cursor_x, cursor_y, bg);

	u32 row_bytes = fb_pitch * FontHeight;
	u32 scroll_bytes = (u32)lines * row_bytes;
	u32 total_bytes = fb_pitch * fb_height;
	u8 *fb = (u8 *)fb_addr;

	memmove(fb, fb + scroll_bytes, total_bytes - scroll_bytes);

	int clear_start_y = (int)(fb_height - (u32)lines * FontHeight);
	for (int y = clear_start_y; y < (int)fb_height; y++) {
		for (int x = 0; x < (int)fb_width; x++) {
			fb_put_pixel(x, y, bg);
		}
	}
}

static void fbcon_cursor_task(void) {
	for (;;) {
		/* 250ms on + 250ms off = 500ms full blink cycle */
		timer_sleep_ms(250);

		bool was_enabled = cpu_irqs_enabled();
		cpu_disable_irqs();

		/* don't blink while the user is actively typing (300ms grace) */
		if (timer_get_ticks() - last_key_tick >= 30) {
			cursor_on = !cursor_on;
			draw_cursor_bar(cursor_x, cursor_y, cursor_on ? 0xAAAAAA : 0x000000);
		}

		if (was_enabled) {
			cpu_enable_irqs();
		}
	}
}

static void fbcon_set_cursor(int x, int y) {
	bool was_enabled = cpu_irqs_enabled();
	cpu_disable_irqs();

	draw_cursor_bar(cursor_x, cursor_y, 0x000000);
	cursor_x = x;
	cursor_y = y;
	cursor_on = true;
	last_key_tick = timer_get_ticks();
	draw_cursor_bar(cursor_x, cursor_y, 0xAAAAAA);

	if (was_enabled) {
		cpu_enable_irqs();
	}

	if (!blink_started && SchedCurrent() != NULL) {
		blink_started = true;
		SchedCreate("fbcon", (u32)fbcon_cursor_task, (u32)(cursor_stack + CursorStackSize));
	}
}

static void fbcon_hw_init(void) {
	fbcon_clear(0xAAAAAA, 0x000000);
	draw_cursor_bar(0, 0, 0xAAAAAA);
}

static struct console_driver fbcon_driver = {
	.cols = 80,
	.rows = 25,
	.init = fbcon_hw_init,
	.putc = fbcon_putc,
	.clear = fbcon_clear,
	.scroll = fbcon_scroll,
	.set_cursor = fbcon_set_cursor,
};

bool fbcon_init(struct boot_info *boot) {
	if (!boot || !boot->framebuffer_available || !boot->framebuffer_addr) {
		return false;
	}
	if (boot->framebuffer_addr > (u64)(__UINTPTR_TYPE__)-1) {
		return false;
	}
	if (boot->framebuffer_bpp != 24 && boot->framebuffer_bpp != 32) {
		return false;
	}

	fb_addr = (volatile u32 *)(__UINTPTR_TYPE__)boot->framebuffer_addr;
	fb_pitch = boot->framebuffer_pitch;
	fb_width = boot->framebuffer_width;
	fb_height = boot->framebuffer_height;
	fb_bpp = boot->framebuffer_bpp;

	fbcon_driver.cols = (int)(fb_width / FontWidth);
	fbcon_driver.rows = (int)(fb_height / FontHeight);

	console_init(&fbcon_driver);
	return true;
}
