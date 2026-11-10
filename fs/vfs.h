#ifndef NOVIUM_FS_VFS_H
#define NOVIUM_FS_VFS_H

#include <novium/types.h>

/* Forward declarations: we use pointers to these before defining them. */
struct vfs_node;
struct vfs_file;
struct vfs_stat;

/*
 * vfs_ops_t
 * 
 * The Virtual File System (VFS) operations table. This structure acts as an 
 * abstract interface (polymorphism in C) using function pointers. 
 * Every underlying filesystem driver (such as a RAM disk, ext2, or FAT) must 
 * populate this table with its own concrete implementation functions. 
 * The kernel can then interact with any storage medium seamlessly using these pointers.
 */
typedef struct vfs_ops {
	struct vfs_node *(*mount)(void);
	int (*open)(struct vfs_node *node, const char *path, struct vfs_file **file);
	int (*close)(struct vfs_file *file);
	ssize_t (*read)(struct vfs_file *file, void *buf, size_t count);
	ssize_t (*write)(struct vfs_file *file, const void *buf, size_t count);
	int (*readdir)(struct vfs_file *file, char *name, size_t name_max);
	int (*stat)(struct vfs_node *node, const char *path, struct vfs_stat *st);
} vfs_ops_t;

/*
 * vfs_node_t
 * Represents one file OR one folder in the tree.
 */
typedef struct vfs_node {
    const vfs_ops_t *ops;
    void *fs_private;
    char name[64];
    struct vfs_node *parent;
    struct vfs_node *child;
    struct vfs_node *sibling;
    u32 flags;
    u32 size;
} vfs_node_t;

/*
 * vfs_file_t
 * Represents an open file (a "handle"). Keeps track of read position.
 */
typedef struct vfs_file {
    vfs_node_t *node;
    size_t pos;
    void *file_private;
} vfs_file_t;

/*
 * vfs_stat_t
 * Returned by vfs_stat() to describe a file or folder.
 */
typedef struct vfs_stat {
    u32 type;
    u32 size;
} vfs_stat_t;

/* Node type flags */
#define VFS_NODE_DIR   1
#define VFS_NODE_FILE  2

/* Stat type flags */
#define VFS_STAT_DIR   1
#define VFS_STAT_FILE  2

/*
 * Public VFS API. 
 */
int vfs_init(void);
int vfs_mount(const char *mountpoint, const vfs_ops_t *ops, void *fs_private);
vfs_node_t *vfs_lookup(const char *path);
int vfs_open(const char *path, vfs_file_t **file);
int vfs_close(vfs_file_t *file);
ssize_t vfs_read(vfs_file_t *file, void *buf, size_t count);
ssize_t vfs_write(vfs_file_t *file, const void *buf, size_t count);
int vfs_readdir(vfs_file_t *file, char *name, size_t name_max);
int vfs_stat(const char *path, vfs_stat_t *st);

#endif