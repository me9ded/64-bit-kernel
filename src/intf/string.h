#pragma once

#include <stddef.h>

/*
 * Kernel versions of the standard C string/memory functions (there is no libc).
 * They must keep these exact names and signatures: gcc may emit calls to
 * memset/memcpy/memmove/memcmp on its own, e.g. for large struct copies.
 */
void* memset(void* dest, int value, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void* memmove(void* dest, const void* src, size_t count);
int memcmp(const void* a, const void* b, size_t count);

size_t strlen(const char* string);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t count);

/* First occurrence of `needle` in `haystack`, or NULL. An empty needle matches at the start. */
char* strstr(const char* haystack, const char* needle);
