#ifndef NOVIUM_FS_INITRD_H
#define NOVIUM_FS_INITRD_H

#include <novium/types.h>

/*
 * tar_header_t
 * Standard 512-byte POSIX ustar header placed before each file in a tar archive.
 */
typedef struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} __attribute__((packed)) tar_header_t;


/*
 * Public initrd API.
 */
int initrd_init(const char *mountpoint, const void *start, size_t size);

#endif
