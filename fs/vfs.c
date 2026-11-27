#include <novium/heap.h>
#include <novium/string.h>
#include <novium/vfs.h>

static vfs_node_t *vfs_root = NULL;

/* Helper: allocate and zero-initialize a vfs_node_t */
static vfs_node_t *vfs_alloc_node(const char *name, u32 flags, const vfs_ops_t *ops, void *fs_private) {
	vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
	if (!node) {
		return NULL;
	}
	memset(node, 0, sizeof(vfs_node_t));
	node->ops = ops;
	node->fs_private = fs_private;
	node->flags = flags;
	if (name) {
		size_t i = 0;
		while (name[i] && i < sizeof(node->name) - 1) {
			node->name[i] = name[i];
			i++;
		}
		node->name[i] = '\0';
	}
	return node;
}

/* Helper: attach a child node to a parent directory node */
static void vfs_add_child(vfs_node_t *parent, vfs_node_t *child) {
	if (!parent || !child) {
		return;
	}
	child->parent = parent;
	child->sibling = parent->child;
	parent->child = child;
}

/* Helper: find a direct child of `dir` matching `name[0..len-1]` */
static vfs_node_t *vfs_find_child_n(vfs_node_t *dir, const char *name, size_t len) {
	if (!dir) {
		return NULL;
	}
	for (vfs_node_t *cur = dir->child; cur != NULL; cur = cur->sibling) {
		size_t cur_len = strlen(cur->name);
		if (cur_len == len && memcmp(cur->name, name, len) == 0) {
			return cur;
		}
	}
	return NULL;
}

int vfs_init(void) {
	vfs_root = vfs_alloc_node("/", VFS_NODE_DIR, NULL, NULL);
	return vfs_root ? 0 : -1;
}

int vfs_mount(const char *mountpoint, const vfs_ops_t *ops, void *fs_private) {
	if (!vfs_root || !mountpoint || !ops) {
		return -1;
	}

	vfs_node_t *mounted = ops->mount ? ops->mount() : NULL;

	/* Mounting at "/" replaces or attaches ops directly to root */
	if (strcmp(mountpoint, "/") == 0) {
		if (mounted) {
			mounted->name[0] = '/';
			mounted->name[1] = '\0';
			mounted->parent = NULL;
			mounted->child = vfs_root->child;
			if (fs_private && !mounted->fs_private) {
				mounted->fs_private = fs_private;
			}
			kfree(vfs_root);
			vfs_root = mounted;
		} else {
			vfs_root->ops = ops;
			vfs_root->fs_private = fs_private;
		}
		return 0;
	}

	/* Resolve parent directory and final mountpoint name (e.g., "/mnt" -> parent "/", name "mnt") */
	const char *last_slash = NULL;
	for (const char *p = mountpoint; *p; p++) {
		if (*p == '/') {
			last_slash = p;
		}
	}
	if (!last_slash || *(last_slash + 1) == '\0') {
		return -1;
	}

	vfs_node_t *parent = vfs_root;
	if (last_slash != mountpoint) {
		char parent_path[256];
		size_t plen = (size_t)(last_slash - mountpoint);
		if (plen >= sizeof(parent_path)) {
			return -1;
		}
		memcpy(parent_path, mountpoint, plen);
		parent_path[plen] = '\0';
		parent = vfs_lookup(parent_path);
		if (!parent || !(parent->flags & VFS_NODE_DIR)) {
			return -1;
		}
	}

	const char *dir_name = last_slash + 1;
	vfs_node_t *target = vfs_find_child_n(parent, dir_name, strlen(dir_name));

	if (!target) {
		target = mounted ? mounted : vfs_alloc_node(dir_name, VFS_NODE_DIR, ops, fs_private);
		if (!target) {
			return -1;
		}
		size_t i = 0;
		while (dir_name[i] && i < sizeof(target->name) - 1) {
			target->name[i] = dir_name[i];
			i++;
		}
		target->name[i] = '\0';
		if (fs_private && !target->fs_private) {
			target->fs_private = fs_private;
		}
		vfs_add_child(parent, target);
	} else {
		target->ops = ops;
		target->fs_private = mounted ? mounted->fs_private : fs_private;
		if (mounted) {
			kfree(mounted);
		}
	}

	return 0;
}

vfs_node_t *vfs_lookup(const char *path) {
	if (!vfs_root || !path || path[0] != '/') {
		return NULL;
	}

	vfs_node_t *cur = vfs_root;
	const char *p = path;

	while (*p) {
		while (*p == '/') {
			p++;
		}
		if (*p == '\0') {
			break;
		}

		const char *start = p;
		while (*p && *p != '/') {
			p++;
		}
		size_t len = (size_t)(p - start);

		if (len == 1 && start[0] == '.') {
			continue;
		}
		if (len == 2 && start[0] == '.' && start[1] == '.') {
			if (cur->parent) {
				cur = cur->parent;
			}
			continue;
		}

		vfs_node_t *next = vfs_find_child_n(cur, start, len);
		if (!next) {
			/* If `cur` is a mounted filesystem root, let its driver resolve subpaths */
			if (cur->ops) {
				return cur;
			}
			return NULL;
		}
		cur = next;
	}

	return cur;
}

int vfs_open(const char *path, vfs_file_t **file) {
	if (!path || !file) {
		return -1;
	}

	vfs_node_t *node = vfs_lookup(path);
	if (!node || !node->ops || !node->ops->open) {
		return -1;
	}

	return node->ops->open(node, path, file);
}

int vfs_close(vfs_file_t *file) {
	if (!file || !file->node) {
		return -1;
	}

	if (file->node->ops && file->node->ops->close) {
		return file->node->ops->close(file);
	}

	kfree(file);
	return 0;
}

ssize_t vfs_read(vfs_file_t *file, void *buf, size_t count) {
	if (!file || !buf || !file->node || !file->node->ops || !file->node->ops->read) {
		return -1;
	}
	return file->node->ops->read(file, buf, count);
}

ssize_t vfs_write(vfs_file_t *file, const void *buf, size_t count) {
	if (!file || !buf || !file->node || !file->node->ops || !file->node->ops->write) {
		return -1;
	}
	return file->node->ops->write(file, buf, count);
}

int vfs_readdir(vfs_file_t *file, char *name, size_t name_max) {
	if (!file || !name || name_max == 0 || !file->node || !file->node->ops || !file->node->ops->readdir) {
		return -1;
	}
	return file->node->ops->readdir(file, name, name_max);
}

int vfs_stat(const char *path, vfs_stat_t *st) {
	if (!path || !st) {
		return -1;
	}

	vfs_node_t *node = vfs_lookup(path);
	if (!node) {
		return -1;
	}

	if (node->ops && node->ops->stat) {
		return node->ops->stat(node, path, st);
	}

	st->type = (node->flags & VFS_NODE_DIR) ? VFS_STAT_DIR : VFS_STAT_FILE;
	st->size = node->size;
	return 0;
}