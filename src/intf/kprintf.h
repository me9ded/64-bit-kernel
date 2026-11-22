#pragma once

#include <stdarg.h>
#include <stddef.h>

/*
 * printf for the kernel.
 *
 * Supported: %c %s %d %i %u %x %X %p %%
 *   flags/width: "-" (left-align), "0" (zero-pad), e.g. %08x, %-10s
 *   length: l, ll, z  (e.g. %lu, %llx, %zu)
 */

/* Print to the screen and the serial port. Returns the number of characters written. */
__attribute__((format(printf, 1, 2)))
int kprintf(const char* format, ...);
int vkprintf(const char* format, va_list args);

/*
 * Format into `buffer` (at most size-1 characters, always NUL-terminated when size > 0).
 * Like C's snprintf, returns the length the full string would have had, so
 * a return value >= size means the output was truncated.
 */
__attribute__((format(printf, 3, 4)))
int ksnprintf(char* buffer, size_t size, const char* format, ...);
int kvsnprintf(char* buffer, size_t size, const char* format, va_list args);

/* Print "KERNEL PANIC: <message>" in white on red, then halt the CPU forever. */
__attribute__((noreturn, format(printf, 1, 2)))
void panic(const char* format, ...);

/*
 * Optional hook called by panic() after the message is printed, before halting.
 * The normal kernel doesn't define it; the test kernel uses it to report panics.
 */
__attribute__((weak)) void panic_hook(void);
