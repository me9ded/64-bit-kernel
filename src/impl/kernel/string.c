#include <string.h>
#include <stdint.h>

void* memset(void* dest, int value, size_t count) {
    uint8_t* d = dest;
    for (size_t i = 0; i < count; i++) {
        d[i] = (uint8_t) value;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

/* Like memcpy, but correct when the two regions overlap. */
void* memmove(void* dest, const void* src, size_t count) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    if (d < s) {
        for (size_t i = 0; i < count; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        for (size_t i = count; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void* a, const void* b, size_t count) {
    const uint8_t* x = a;
    const uint8_t* y = b;
    for (size_t i = 0; i < count; i++) {
        if (x[i] != y[i]) {
            return x[i] < y[i] ? -1 : 1;
        }
    }
    return 0;
}

size_t strlen(const char* string) {
    size_t length = 0;
    while (string[length] != '\0') {
        length++;
    }
    return length;
}

int strcmp(const char* a, const char* b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char) *a - (unsigned char) *b;
}

int strncmp(const char* a, const char* b, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (a[i] != b[i] || a[i] == '\0') {
            return (unsigned char) a[i] - (unsigned char) b[i];
        }
    }
    return 0;
}
