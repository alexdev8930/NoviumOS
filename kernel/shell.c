#include <novium/shell.h>
#include <novium/stdio.h>
#include <novium/string.h>
#include <novium/drivers/console.h>
#include <novium/drivers/input.h>
#include <novium/sched.h>
#include <stdbool.h>

#define SHELL_LINE_MAX 256
#define SHELL_MAX_ARGS 16

static int shell_parse(char *line, char **argv) {
	int argc = 0;
	char *p = line;

	while (*p != '\0' && argc < SHELL_MAX_ARGS) {
		while (*p == ' ' || *p == '\t') {
			*p++ = '\0';
		}
		if (*p == '\0') {
			break;
		}
		argv[argc++] = p;
		while (*p != '\0' && *p != ' ' && *p != '\t') {
			p++;
		}
	}
	return argc;
}

void shell_task(void) {
	char line[SHELL_LINE_MAX];
	char *argv[SHELL_MAX_ARGS];
	size_t len = 0;

	shell_prompt();

	for (;;) {
		int key = keyboard_getchar();

		if (key < 0) {
			SchedYield();
			continue;
		}

		console_line_echo = true;
		console_putchar((char)key);
		console_line_echo = false;

		if (key == '\b') {
			if (len > 0) {
				len--;
			}
		} else if (key == '\n') {
			line[len] = '\0';
			int argc = shell_parse(line, argv);
			if (argc > 0) {
				shell_exec(argc, argv);
			}
			len = 0;
			shell_prompt();
		} else if (len < SHELL_LINE_MAX - 1) {
			line[len++] = (char)key;
		} else {
			console_putchar('\a'); /* Beep to signal the line is full. */
		}
	}
}
