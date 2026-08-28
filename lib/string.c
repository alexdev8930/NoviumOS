#include <novium/string.h>

void *memset(void *s, int c, size_t len) {
    unsigned char *ptr = (unsigned char *)s;
    while (len--) {
        *ptr++ = (unsigned char)c;
    }
    return s;
}


void *memmove(void *dest, const void *src, size_t len) 
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d < s) {
        while (len--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += len;
        s += len;
        while (len--) {
            *--d = *--s;
        }
    }

    return dest;
}



void *memcpy(void *dest, const void *src, size_t len) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s_ptr = (const unsigned char *)src;
    while (len--) {
        *d++ = *s_ptr++;
    }
    return dest;
}


size_t strlen(const char *str) {
    size_t len = 0;

    while (str[len] != '\0') {
        len++;
    }
    return len;
}


int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}