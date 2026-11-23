#pragma once

/*
 * Minimal in-kernel test framework.
 *
 * Write a test in any .c file under tests/kernel/:
 *
 *     TEST(my_feature_works) {
 *         CHECK(1 + 1 == 2);
 *         CHECK_EQ_INT(add(2, 3), 5);
 *         CHECK_EQ_STR(buffer, "expected");
 *     }
 *
 * It is registered automatically (the linker collects every TEST into the
 * .kernel_tests section) and run by the test kernel (tests/kernel/test_main.c).
 * Results go to the serial port only, so tests can inspect the screen freely.
 */

#include <cpu.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct test_case {
    const char* name;
    void (*run)(void);
};

#define TEST(name)                                                              \
    static void name(void);                                                     \
    static const struct test_case test_case_##name                              \
        __attribute__((used, section(".kernel_tests"), aligned(8))) = { #name, name }; \
    static void name(void)

/* Record a failure (the test keeps running so you see every failed check). */
__attribute__((format(printf, 3, 4)))
void test_fail(const char* file, int line, const char* format, ...);

/* Write a line to the serial port (not the screen). */
__attribute__((format(printf, 1, 2)))
void test_log(const char* format, ...);

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            test_fail(__FILE__, __LINE__, "CHECK(%s)", #condition);             \
        }                                                                       \
    } while (0)

#define CHECK_EQ_INT(actual, expected)                                          \
    do {                                                                        \
        long long actual_ = (long long) (actual);                               \
        long long expected_ = (long long) (expected);                           \
        if (actual_ != expected_) {                                             \
            test_fail(__FILE__, __LINE__, "%s: got %lld, expected %lld",        \
                      #actual, actual_, expected_);                             \
        }                                                                       \
    } while (0)

#define CHECK_EQ_STR(actual, expected)                                          \
    do {                                                                        \
        const char* actual_ = (actual);                                         \
        const char* expected_ = (expected);                                     \
        if (strcmp(actual_, expected_) != 0) {                                  \
            test_fail(__FILE__, __LINE__, "%s: got \"%s\", expected \"%s\"",    \
                      #actual, actual_, expected_);                             \
        }                                                                       \
    } while (0)

/* Read the VGA text screen (80x25) directly. */
static inline uint16_t vga_cell(size_t row, size_t col) {
    return ((volatile uint16_t*) 0xb8000)[row * 80 + col];
}
static inline char vga_char(size_t row, size_t col) {
    return (char) (vga_cell(row, col) & 0xff);
}
static inline uint8_t vga_color(size_t row, size_t col) {
    return (uint8_t) (vga_cell(row, col) >> 8);
}

/* Copy screen row `row` into `out` as a string, without trailing spaces. */
static inline void vga_row_text(size_t row, char* out, size_t size) {
    size_t length = 0;
    for (size_t col = 0; col < 80 && length + 1 < size; col++) {
        out[length++] = vga_char(row, col);
    }
    while (length > 0 && out[length - 1] == ' ') {
        length--;
    }
    out[length] = '\0';
}

/* True if `text` appears anywhere on screen (within a single row). */
static inline int vga_contains(const char* text) {
    char row[81];
    for (size_t r = 0; r < 25; r++) {
        vga_row_text(r, row, sizeof row);
        if (strstr(row, text) != NULL) {
            return 1;
        }
    }
    return 0;
}

/* Hardware cursor position (row * 80 + col), read back from the VGA controller. */
static inline uint16_t vga_cursor(void) {
    outb(0x3D4, 0x0F);
    uint16_t low = inb(0x3D5);
    outb(0x3D4, 0x0E);
    uint16_t high = inb(0x3D5);
    return (uint16_t) ((high << 8) | low);
}
