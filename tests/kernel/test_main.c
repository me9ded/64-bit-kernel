/*
 * Test kernel entry point: replaces src/impl/kernel/main.c in the test build.
 * Runs every TEST(...), reports over the serial port, then exits QEMU with a
 * status code that tests/run.sh checks.
 */
#include "test.h"
#include <cpu.h>
#include <kprintf.h>
#include <print.h>
#include <serial.h>
#include <stdbool.h>

/*
 * QEMU's isa-debug-exit device (-device isa-debug-exit,iobase=0xf4,iosize=0x04):
 * writing value V to port 0xf4 makes QEMU exit with status (V << 1) | 1.
 */
#define QEMU_EXIT_PORT    0xf4
#define QEMU_EXIT_SUCCESS 0x10 /* QEMU exit status 33 */
#define QEMU_EXIT_FAILURE 0x11 /* QEMU exit status 35 */

extern const struct test_case __tests_start[];
extern const struct test_case __tests_end[];

static int current_test_failures;
static int tests_passed;
static bool expecting_panic;

__attribute__((noreturn)) static void qemu_exit(uint8_t code) {
    outb(QEMU_EXIT_PORT, code);
    halt_forever(); /* only reached if the exit device is missing */
}

void test_log(const char* format, ...) {
    char line[256];
    va_list args;
    va_start(args, format);
    kvsnprintf(line, sizeof line, format, args);
    va_end(args);
    serial_write_str(line);
}

void test_fail(const char* file, int line, const char* format, ...) {
    char message[200];
    va_list args;
    va_start(args, format);
    kvsnprintf(message, sizeof message, format, args);
    va_end(args);

    test_log("    %s:%d: %s\n", file, line, message);
    current_test_failures++;
}

/*
 * Last test: panic() never returns, so it runs after all the others. Its
 * message must appear on screen in white on red, then panic() calls panic_hook().
 */
static const char* const PANIC_LINE = "KERNEL PANIC: test panic 123";

void panic_hook(void) {
    if (!expecting_panic) {
        test_log("FAIL unexpected kernel panic (message above)\n");
        qemu_exit(QEMU_EXIT_FAILURE);
    }

    /* print_clear() put the cursor at (0,0); panic printed "\n" first, so row 1 */
    bool ok = true;
    for (size_t i = 0; PANIC_LINE[i] != '\0'; i++) {
        if (vga_char(1, i) != PANIC_LINE[i] || vga_color(1, i) != 0x4f) {
            test_log("    screen row 1, col %zu: got '%c' color 0x%02x, expected '%c' color 0x4f\n",
                     i, vga_char(1, i), vga_color(1, i), PANIC_LINE[i]);
            ok = false;
            break;
        }
    }

    if (!ok) {
        test_log("FAIL panic_shows_message_in_white_on_red\n");
        qemu_exit(QEMU_EXIT_FAILURE);
    }
    test_log("PASS panic_shows_message_in_white_on_red\n");
    test_log("=== ALL %d TESTS PASSED ===\n", tests_passed + 1);
    qemu_exit(QEMU_EXIT_SUCCESS);
}

void kernel_main(void) {
    if (serial_init() != 0) {
        qemu_exit(QEMU_EXIT_FAILURE); /* can't report anything without serial */
    }

    test_log("=== kernel tests ===\n");
    int failed = 0;

    for (const struct test_case* test = __tests_start; test < __tests_end; test++) {
        current_test_failures = 0;
        test->run();
        if (current_test_failures == 0) {
            tests_passed++;
            test_log("PASS %s\n", test->name);
        } else {
            failed++;
            test_log("FAIL %s\n", test->name);
        }
    }

    if (failed > 0) {
        test_log("=== %d passed, %d FAILED ===\n", tests_passed, failed);
        qemu_exit(QEMU_EXIT_FAILURE);
    }

    expecting_panic = true;
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_clear();
    panic("test panic %d", 123); /* continues in panic_hook() */
}
