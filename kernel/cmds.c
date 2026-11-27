#include <novium/drivers/vt.h>
#include <novium/shell.h>
#include <novium/vfs.h>
#include <novium/stdio.h>
#include <novium/string.h>
#include <stdbool.h>

typedef struct ShellCommand {
	const char *name;
	const char *desc;
	void (*fn)(int argc, char **argv);
} ShellCommand;

static char cwd[256] = "/";
static char hostname[64] = "novium";
static bool hostname_loaded = false;

static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_pwd(int argc, char **argv);
static void cmd_cd(int argc, char **argv);
static void cmd_ls(int argc, char **argv);
static void cmd_cat(int argc, char **argv);
static void cmd_stat(int argc, char **argv);

static const ShellCommand commands[] = {
	{ "help",  "List available commands",      cmd_help  },
	{ "clear", "Clear the console screen",     cmd_clear },
	{ "echo",  "Print arguments to console",   cmd_echo  },
	{ "pwd",   "Print working directory",      cmd_pwd   },
	{ "cd",    "Change working directory",     cmd_cd    },
	{ "ls",    "List directory contents",      cmd_ls    },
	{ "cat",   "Print file contents",          cmd_cat   },
	{ "stat",  "Show file or folder info",     cmd_stat  },
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void shell_load_hostname(void) {
	vfs_file_t *file = NULL;
	if (vfs_open("/etc/hostname", &file) < 0 || file == NULL) {
		return;
	}

	ssize_t n = vfs_read(file, hostname, sizeof(hostname) - 1);
	vfs_close(file);

	if (n > 0) {
		hostname[n] = '\0';
		while (n > 0 && (hostname[n - 1] == '\n' || hostname[n - 1] == '\r' || hostname[n - 1] == ' ')) {
			hostname[--n] = '\0';
		}
	}
}

void shell_prompt(void) {
	if (!hostname_loaded) {
		shell_load_hostname();
		hostname_loaded = true;
	}
	kprintf("root@%s:%s$ ", hostname, cwd);
	user_cmdline_start = console_cursor();
}

/*
 * glue relative paths onto cwd and clean up "." and ".." so the vfs
 * always gets a normal absolute path starting at /
 */
static void resolve_path(const char *input, char *out, size_t out_max) {
	char raw[256];
	size_t raw_len = 0;

	if (input[0] == '/') {
		while (input[raw_len] != '\0' && raw_len + 1 < sizeof(raw)) {
			raw[raw_len] = input[raw_len];
			raw_len++;
		}
		raw[raw_len] = '\0';
	} else {
		size_t cwd_len = strlen(cwd);
		if (cwd_len >= sizeof(raw)) {
			cwd_len = sizeof(raw) - 1;
		}
		memcpy(raw, cwd, cwd_len);
		raw_len = cwd_len;

		if (raw_len > 0 && raw[raw_len - 1] != '/' && raw_len + 1 < sizeof(raw)) {
			raw[raw_len++] = '/';
		}

		size_t i = 0;
		while (input[i] != '\0' && raw_len + 1 < sizeof(raw)) {
			raw[raw_len++] = input[i++];
		}
		raw[raw_len] = '\0';
	}

	/* walk the path segments and pop back one level whenever we hit ".." */
	char *parts[32];
	int depth = 0;
	char *p = raw;

	while (*p != '\0') {
		while (*p == '/') {
			*p++ = '\0';
		}
		if (*p == '\0') {
			break;
		}
		char *seg = p;
		while (*p != '\0' && *p != '/') {
			p++;
		}
		if (*p == '/') {
			*p++ = '\0';
		}

		if (strcmp(seg, ".") == 0) {
			continue;
		} else if (strcmp(seg, "..") == 0) {
			if (depth > 0) {
				depth--;
			}
		} else if (depth < 32) {
			parts[depth++] = seg;
		}
	}

	if (depth == 0) {
		if (out_max >= 2) {
			out[0] = '/';
			out[1] = '\0';
		}
		return;
	}

	size_t pos = 0;
	for (int i = 0; i < depth; i++) {
		if (pos + 1 < out_max) {
			out[pos++] = '/';
		}
		size_t slen = strlen(parts[i]);
		for (size_t j = 0; j < slen && pos + 1 < out_max; j++) {
			out[pos++] = parts[i][j];
		}
	}
	out[pos] = '\0';
}

static int print_file(const char *path) {
	vfs_file_t *file = NULL;
	int rc = vfs_open(path, &file);
	if (rc < 0 || file == NULL) {
		return rc;
	}

	char buf[128];
	for (;;) {
		ssize_t n = vfs_read(file, buf, sizeof(buf) - 1);
		if (n <= 0) {
			break;
		}
		buf[n] = '\0';
		kprintf("%s", buf);
	}

	vfs_close(file);
	return 0;
}

void shell_print_motd(void) {
	console_clear();
	if (print_file("/etc/motd") < 0) {
		kprintf("Welcome to Novium OS!\n");
	}
	kprintf("\n");
}

static void cmd_help(int argc, char **argv) {
	(void)argc;
	(void)argv;

	if (print_file("/usr/share/help.txt") == 0) {
		return;
	}

	kprintf("Available commands:\n");
	for (size_t i = 0; i < NUM_COMMANDS; i++) {
		kprintf("  %-8s - %s\n", commands[i].name, commands[i].desc);
	}
}

static void cmd_clear(int argc, char **argv) {
	(void)argc;
	(void)argv;
	console_clear();
}

static void cmd_echo(int argc, char **argv) {
	for (int i = 1; i < argc; i++) {
		kprintf("%s", argv[i]);
		if (i + 1 < argc) {
			kprintf(" ");
		}
	}
	kprintf("\n");
}

static void cmd_pwd(int argc, char **argv) {
	(void)argc;
	(void)argv;
	kprintf("%s\n", cwd);
}

static void cmd_cd(int argc, char **argv) {
	const char *target = (argc > 1) ? argv[1] : "/";
	char resolved[256];
	resolve_path(target, resolved, sizeof(resolved));

	vfs_stat_t st;
	if (vfs_stat(resolved, &st) < 0) {
		kprintf("cd: %s: No such file or directory\n", target);
		return;
	}
	if (st.type != VFS_STAT_DIR) {
		kprintf("cd: %s: Not a directory\n", target);
		return;
	}

	size_t len = strlen(resolved);
	memcpy(cwd, resolved, len + 1);
}

static void cmd_ls(int argc, char **argv) {
	const char *target = (argc > 1) ? argv[1] : cwd;
	char resolved[256];
	resolve_path(target, resolved, sizeof(resolved));

	vfs_stat_t st;
	if (vfs_stat(resolved, &st) < 0) {
		kprintf("ls: %s: No such file or directory\n", target);
		return;
	}
	if (st.type != VFS_STAT_DIR) {
		kprintf("%s\n", target);
		return;
	}

	vfs_file_t *dir = NULL;
	if (vfs_open(resolved, &dir) < 0 || dir == NULL) {
		kprintf("ls: cannot open %s\n", target);
		return;
	}

	char name[64];
	while (vfs_readdir(dir, name, sizeof(name)) > 0) {
		kprintf("%s  ", name);
	}
	kprintf("\n");

	vfs_close(dir);
}

static void cmd_cat(int argc, char **argv) {
	if (argc < 2) {
		kprintf("usage: cat <file>\n");
		return;
	}

	char resolved[256];
	resolve_path(argv[1], resolved, sizeof(resolved));

	vfs_stat_t st;
	if (vfs_stat(resolved, &st) < 0) {
		kprintf("cat: %s: No such file or directory\n", argv[1]);
		return;
	}
	if (st.type == VFS_STAT_DIR) {
		kprintf("cat: %s: Is a directory\n", argv[1]);
		return;
	}

	if (print_file(resolved) < 0) {
		kprintf("cat: failed to read %s\n", argv[1]);
	}
}

static void cmd_stat(int argc, char **argv) {
	if (argc < 2) {
		kprintf("usage: stat <path>\n");
		return;
	}

	char resolved[256];
	resolve_path(argv[1], resolved, sizeof(resolved));

	vfs_stat_t st;
	if (vfs_stat(resolved, &st) < 0) {
		kprintf("stat: %s: No such file or directory\n", argv[1]);
		return;
	}

	const char *kind = (st.type == VFS_STAT_DIR) ? "directory" : "file";
	kprintf("Path: %s\nType: %s\nSize: %u bytes\n", resolved, kind, st.size);
}

void shell_exec(int argc, char **argv) {
	for (size_t i = 0; i < NUM_COMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0) {
			commands[i].fn(argc, argv);
			return;
		}
	}
	kprintf("Unknown command: %s (type 'help' for a list of commands)\n", argv[0]);
}
