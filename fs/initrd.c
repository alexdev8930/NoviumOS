#include <novium/initrd.h>
#include <novium/vfs.h>
#include <novium/heap.h>
#include <novium/string.h>

typedef struct initrd_fs {
    const u8 *base;
    size_t size;
} initrd_fs_t;

typedef struct initrd_file_priv {
    const u8 *data;
    u32 size;
    u32 dir_offset;
    bool is_dir;
    char dir_prefix[128];
} initrd_file_priv_t;

static initrd_fs_t initrd_state;

/* Helper: parse ASCII octal size field from a 512-byte tar header */
static u32 tar_parse_octal(const char *str, size_t len) {
    u32 val = 0;
    for (size_t i = 0; i < len && str[i] >= '0' && str[i] <= '7'; i++) {
        val = (val << 3) | (u32)(str[i] - '0');
    }
    return val;
}

/* Helper: strip leading "./" or "/" from tar entry names */
static const char *tar_clean_name(const char *name) {
    if (name[0] == '.' && name[1] == '/') {
        name += 2;
    }
    while (*name == '/') {
        name++;
    }
    return name;
}


static bool tar_name_matches(const char *tar_clean, const char *rel) {
    size_t tlen = strlen(tar_clean);
    while (tlen > 0 && tar_clean[tlen - 1] == '/') {
        tlen--;
    }
    size_t rlen = strlen(rel);
    while (rlen > 0 && rel[rlen - 1] == '/') {
        rlen--;
    }
    return (tlen == rlen) && (strncmp(tar_clean, rel, tlen) == 0);
}

/* Helper: locate a tar header by relative path inside the archive */
static const tar_header_t *tar_find(const initrd_fs_t *fs, const char *rel, u32 *out_size) {
    size_t off = 0;
    while (off + sizeof(tar_header_t) <= fs->size) {
        const tar_header_t *hdr = (const tar_header_t *)(fs->base + off);
        if (hdr->name[0] == '\0') {
            break;
        }
        u32 fsize = tar_parse_octal(hdr->size, sizeof(hdr->size));
        const char *clean = tar_clean_name(hdr->name);
        if (*clean != '\0' && tar_name_matches(clean, rel)) {
            if (out_size) {
                *out_size = fsize;
            }
            return hdr;
        }
        size_t blocks = (fsize + 511u) / 512u;
        off += 512u + blocks * 512u;
    }
    return NULL;
}

static int initrd_open(struct vfs_node *node, const char *path, struct vfs_file **file) {
    if (!node || !node->fs_private || !path || !file) {
        return -1;
    }

    initrd_fs_t *fs = (initrd_fs_t *)node->fs_private;
    const char *rel = path;
    while (*rel == '/') {
        rel++;
    }

    initrd_file_priv_t *priv = (initrd_file_priv_t *)kmalloc(sizeof(initrd_file_priv_t));
    if (!priv) {
        return -1;
    }
    memset(priv, 0, sizeof(initrd_file_priv_t));

    if (*rel == '\0') {
        priv->is_dir = true;
        priv->dir_offset = 0;
        priv->dir_prefix[0] = '\0';
    } else {
        u32 fsize = 0;
        const tar_header_t *hdr = tar_find(fs, rel, &fsize);
        if (!hdr) {
            kfree(priv);
            return -1;
        }
        priv->is_dir = (hdr->typeflag == '5');
        priv->data = (const u8 *)hdr + 512u;
        priv->size = fsize;
        if (priv->is_dir) {
            size_t rlen = strlen(rel);
            while (rlen > 0 && rel[rlen - 1] == '/') {
                rlen--;
            }
            memcpy(priv->dir_prefix, rel, rlen);
            priv->dir_prefix[rlen] = '/';
            priv->dir_prefix[rlen + 1] = '\0';
        }
    }

    vfs_file_t *f = (vfs_file_t *)kmalloc(sizeof(vfs_file_t));
    if (!f) {
        kfree(priv);
        return -1;
    }
    memset(f, 0, sizeof(vfs_file_t));
    f->node = node;
    f->file_private = priv;
    *file = f;
    return 0;
}

static int initrd_close(struct vfs_file *file) {
    if (!file) {
        return -1;
    }
    if (file->file_private) {
        kfree(file->file_private);
    }
    kfree(file);
    return 0;
}

static ssize_t initrd_read(struct vfs_file *file, void *buf, size_t count) {
    if (!file || !buf || !file->file_private) {
        return -1;
    }
    initrd_file_priv_t *priv = (initrd_file_priv_t *)file->file_private;
    if (priv->is_dir || !priv->data) {
        return -1;
    }
    if (file->pos >= priv->size) {
        return 0;
    }
    size_t avail = priv->size - file->pos;
    size_t to_read = (count < avail) ? count : avail;
    memcpy(buf, priv->data + file->pos, to_read);
    file->pos += to_read;
    return (ssize_t)to_read;
}

static bool extract_child(const char *clean, const char *prefix, size_t plen, char *out, size_t max) {
    if (plen > 0 && strncmp(clean, prefix, plen) != 0) {
        return false;
    }
    const char *sub = clean + plen;
    if (*sub == '\0') {
        return false;
    }
    size_t i = 0;
    while (sub[i] && sub[i] != '/' && i < max - 1) {
        out[i] = sub[i];
        i++;
    }
    out[i] = '\0';
    return i > 0;
}

static bool already_seen(const initrd_fs_t *fs, u32 limit, const char *prefix, size_t plen, const char *target) {
    char seen[128];
    size_t off = 0;
    while (off < limit && off + sizeof(tar_header_t) <= fs->size) {
        const tar_header_t *hdr = (const tar_header_t *)(fs->base + off);
        if (hdr->name[0] == '\0') {
            break;
        }
        u32 fsize = tar_parse_octal(hdr->size, sizeof(hdr->size));
        size_t blocks = (fsize + 511u) / 512u;
        off += 512u + blocks * 512u;

        const char *clean = tar_clean_name(hdr->name);
        if (extract_child(clean, prefix, plen, seen, sizeof(seen)) && strcmp(seen, target) == 0) {
            return true;
        }
    }
    return false;
}

static int initrd_readdir(struct vfs_file *file, char *name, size_t name_max) {
    if (!file || !name || name_max == 0 || !file->node || !file->node->fs_private || !file->file_private) {
        return -1;
    }
    initrd_fs_t *fs = (initrd_fs_t *)file->node->fs_private;
    initrd_file_priv_t *priv = (initrd_file_priv_t *)file->file_private;
    if (!priv->is_dir) {
        return -1;
    }

    size_t plen = strlen(priv->dir_prefix);

    while (priv->dir_offset + sizeof(tar_header_t) <= fs->size) {
        u32 cur_off = priv->dir_offset;
        const tar_header_t *hdr = (const tar_header_t *)(fs->base + cur_off);
        if (hdr->name[0] == '\0') {
            return 0;
        }
        u32 fsize = tar_parse_octal(hdr->size, sizeof(hdr->size));
        size_t blocks = (fsize + 511u) / 512u;
        priv->dir_offset += (u32)(512u + blocks * 512u);

        const char *clean = tar_clean_name(hdr->name);
        if (!extract_child(clean, priv->dir_prefix, plen, name, name_max)) {
            continue;
        }
        if (already_seen(fs, cur_off, priv->dir_prefix, plen, name)) {
            continue;
        }
        return 1;
    }
    return 0;
}

static int initrd_stat(struct vfs_node *node, const char *path, struct vfs_stat *st) {
    if (!node || !node->fs_private || !path || !st) {
        return -1;
    }
    initrd_fs_t *fs = (initrd_fs_t *)node->fs_private;
    const char *rel = path;
    while (*rel == '/') {
        rel++;
    }
    if (*rel == '\0') {
        st->type = VFS_STAT_DIR;
        st->size = 0;
        return 0;
    }
    u32 fsize = 0;
    const tar_header_t *hdr = tar_find(fs, rel, &fsize);
    if (!hdr) {
        return -1;
    }
    st->type = (hdr->typeflag == '5') ? VFS_STAT_DIR : VFS_STAT_FILE;
    st->size = fsize;
    return 0;
}

static const vfs_ops_t initrd_ops = {
    .mount = NULL,
    .open = initrd_open,
    .close = initrd_close,
    .read = initrd_read,
    .write = NULL,
    .readdir = initrd_readdir,
    .stat = initrd_stat,
};

int initrd_init(const char *mountpoint, const void *start, size_t size) {
    if (!mountpoint || !start) {
        return -1;
    }
    initrd_state.base = (const u8 *)start;
    initrd_state.size = size;
    return vfs_mount(mountpoint, &initrd_ops, &initrd_state);
}
