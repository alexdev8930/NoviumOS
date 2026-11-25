#include <asm/io.h>
#include <novium/drivers/vt.h>
#include <novium/string.h>

/* Text page geometry: 80x25 cells of two bytes each, at 0xB8000. */
enum { ConsoleWidth = 80, ConsoleHeight = 25, ConsoleCells = ConsoleWidth * ConsoleHeight };

/*
 * VGA text mode palette; an attribute byte is background << 4 | foreground.
 *
 * Packed, because a plain enum is int sized here: four bytes of palette in a
 * text cell would pad it and shift every cell after it by two bytes.
 */
typedef enum __attribute__((packed)) {
	VgaBlack = 0,
	VgaBlue,
	VgaGreen,
	VgaCyan,
	VgaRed,
	VgaMagenta,
	VgaBrown,
	VgaLightGrey,
	VgaDarkGrey,
	VgaLightBlue,
	VgaLightGreen,
	VgaLightCyan,
	VgaLightRed,
	VgaLightMagenta,
	VgaYellow,
	VgaWhite
} VgaColor;

/* Packed picks the smallest type that fits, so an enumerator past 255 would
 * silently widen the palette again. Fail the build instead. */
_Static_assert(sizeof(VgaColor) == 1, "VgaColor grew past a byte, text cells will misalign");

#define VGA_ATTRIBUTE(Background, Foreground) ((u8)((Background) << 4) | (u8)(Foreground))

static char *video_memory = (char *)0xB8000;

static VgaColor rgb_to_vga(u32 rgb)
{
	if (rgb == 0xAA5500)
		return VgaBrown;

	u8 r = (rgb >> 16) & 0xFF;
	u8 g = (rgb >> 8) & 0xFF;
	u8 b = rgb & 0xFF;

	u8 idx = ((r >= 0x80) << 2) | ((g >= 0x80) << 1) | (b >= 0x80);
	if (r > 0xAA || g > 0xAA || b > 0xAA)
		idx |= 0x08;

	return (VgaColor)idx;
}

/* the only place that moves the cursor, so it can never leave the text page */
static void vgacon_set_cursor(int x, int y)
{
	int position = y * ConsoleWidth + x;
	if (position < 0)
		position = 0;
	else if (position >= ConsoleCells)
		position = ConsoleCells - 1;

	/* select low cursor byte register (0x0F) on port 0x3D4, write value to 0x3D5 */
	outb(0x3D4, 0x0F);
	outb(0x3D5, (u8)(position & 0xFF));
	/* select high cursor byte register (0x0E) on port 0x3D4, write value to 0x3D5 */
	outb(0x3D4, 0x0E);
	outb(0x3D5, (u8)((position >> 8) & 0xFF));
}

static void vgacon_putc(int x, int y, char c, u32 fg, u32 bg)
{
	int idx = (y * ConsoleWidth + x) * 2;
	video_memory[idx] = c;
	video_memory[idx + 1] = VGA_ATTRIBUTE(rgb_to_vga(bg), rgb_to_vga(fg));
}

static void vgacon_clear(u32 fg, u32 bg)
{
	u8 attr = VGA_ATTRIBUTE(rgb_to_vga(bg), rgb_to_vga(fg));
	u16 *cells = (u16 *)video_memory;
	u16 blank = ((u16)attr << 8) | (u8)' ';

	for (int i = 0; i < ConsoleCells; i++)
		cells[i] = blank;

	/* GRUB may leave the VGA text cursor disabled. */
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x0E);
	outb(0x3D4, 0x0B);
	outb(0x3D5, 0x0F);
}

static void vgacon_scroll(int lines, u32 fg, u32 bg)
{
	if (lines <= 0)
		return;
	if (lines >= ConsoleHeight) {
		vgacon_clear(fg, bg);
		return;
	}

	size_t row_bytes = ConsoleWidth * 2;
	size_t keep_bytes = row_bytes * (ConsoleHeight - lines);
	memmove(video_memory, video_memory + (lines * row_bytes), keep_bytes);

	u8 attr = VGA_ATTRIBUTE(rgb_to_vga(bg), rgb_to_vga(fg));
	char *bottom = video_memory + keep_bytes;
	for (int i = 0; i < ConsoleWidth * lines; i++) {
		bottom[i * 2] = ' ';
		bottom[i * 2 + 1] = attr;
	}
}

static void vgacon_hw_init(void)
{
	vgacon_clear(0xAAAAAA, 0x000000);
}

static struct console_driver vgacon_driver = {
	.cols       = ConsoleWidth,
	.rows       = ConsoleHeight,
	.init       = vgacon_hw_init,
	.putc       = vgacon_putc,
	.clear      = vgacon_clear,
	.scroll     = vgacon_scroll,
	.set_cursor = vgacon_set_cursor,
};

void vgacon_init(void)
{
	console_init(&vgacon_driver);
}
