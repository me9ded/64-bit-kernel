#include "test.h"
#include <keyboard.h>
#include <print.h>
#include <readline.h>
#include <string.h>

#define SC_ENTER     0x1C
#define SC_BACKSPACE 0x0E
#define SC_CTRL      0x1D

/* Scancodes for lowercase letters a-z */
static const uint8_t letter_scancodes[26] = {
    0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
    0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C,
};

static void tap(uint8_t scancode) {
    keyboard_process_scancode(scancode);
    keyboard_process_scancode(scancode | 0x80);
}

/* Queue key presses for lowercase letters, '\b' (Backspace) and '\n' (Enter). */
static void type(const char* keys) {
    for (size_t i = 0; keys[i] != '\0'; i++) {
        if (keys[i] == '\n') {
            tap(SC_ENTER);
        } else if (keys[i] == '\b') {
            tap(SC_BACKSPACE);
        } else {
            tap(letter_scancodes[keys[i] - 'a']);
        }
    }
}

#define SC_EXTENDED 0xE0
#define SC_UP       0x48
#define SC_DOWN     0x50

static void tap_extended(uint8_t scancode) {
    keyboard_process_scancode(SC_EXTENDED);
    keyboard_process_scancode(scancode);
    keyboard_process_scancode(SC_EXTENDED);
    keyboard_process_scancode(scancode | 0x80);
}

TEST(readline_returns_typed_line) {
    keyboard_reset();
    print_clear();
    type("hello\n");
    char line[32];
    CHECK_EQ_INT(readline(line, sizeof line), 5);
    CHECK_EQ_STR(line, "hello");
    CHECK_EQ_INT(vga_char(0, 0), 'h');
    CHECK_EQ_INT(vga_char(0, 4), 'o');
}

TEST(readline_backspace_edits_the_line) {
    keyboard_reset();
    print_clear();
    type("abc\bd\n");
    char line[32];
    CHECK_EQ_INT(readline(line, sizeof line), 3);
    CHECK_EQ_STR(line, "abd");
    CHECK_EQ_INT(vga_char(0, 2), 'd');       /* 'c' was erased on screen too */
    CHECK_EQ_INT(vga_char(0, 3), ' ');
}

TEST(readline_backspace_stops_at_line_start) {
    keyboard_reset();
    print_clear();
    print_str("> ");
    type("\b\b\bok\n");
    char line[32];
    CHECK_EQ_INT(readline(line, sizeof line), 2);
    CHECK_EQ_STR(line, "ok");
    CHECK_EQ_INT(vga_char(0, 0), '>');       /* the prompt wasn't erased */
}

TEST(readline_ignores_keys_when_buffer_full) {
    keyboard_reset();
    type("abcdef\n");
    char line[4];
    CHECK_EQ_INT(readline(line, sizeof line), 3);
    CHECK_EQ_STR(line, "abc");
}

TEST(readline_empty_line) {
    keyboard_reset();
    type("\n");
    char line[8] = "junk";
    CHECK_EQ_INT(readline(line, sizeof line), 0);
    CHECK_EQ_STR(line, "");
}

TEST(readline_ctrl_c_cancels) {
    keyboard_reset();
    type("abc");
    keyboard_process_scancode(SC_CTRL);
    type("c");
    keyboard_process_scancode(SC_CTRL | 0x80);
    char line[32];
    CHECK_EQ_INT(readline(line, sizeof line), -1);
    CHECK_EQ_STR(line, "");
}

/* Queue `keys`, run readline, and return the line it produced. */
static const char* read_with(const char* keys) {
    static char line[32];
    type(keys);
    readline(line, sizeof line);
    return line;
}

TEST(readline_history_recalls_previous_lines) {
    keyboard_reset();
    readline_clear_history();
    print_clear();
    read_with("one\n");
    read_with("two\n");

    tap_extended(SC_UP);
    CHECK_EQ_STR(read_with("\n"), "two");

    tap_extended(SC_UP);
    tap_extended(SC_UP);
    tap_extended(SC_UP);                   /* history is [one, two]: stops at the oldest */
    CHECK_EQ_STR(read_with("\n"), "one");

    tap_extended(SC_UP);
    tap_extended(SC_DOWN);                 /* past the newest entry: empty line */
    CHECK_EQ_STR(read_with("\n"), "");
}

TEST(readline_history_skips_duplicates_and_empty_lines) {
    keyboard_reset();
    readline_clear_history();
    print_clear();
    read_with("same\n");
    read_with("same\n");
    read_with("\n");

    tap_extended(SC_UP);
    tap_extended(SC_UP);                   /* only one entry was stored */
    CHECK_EQ_STR(read_with("\n"), "same");
}

TEST(readline_history_replaces_typed_text) {
    keyboard_reset();
    readline_clear_history();
    read_with("abc\n");

    print_clear();
    type("xy");
    tap_extended(SC_UP);
    type("\n");
    char line[32];
    CHECK_EQ_INT(readline(line, sizeof line), 3);
    CHECK_EQ_STR(line, "abc");
    char row[81];
    vga_row_text(0, row, sizeof row);
    CHECK_EQ_STR(row, "abc");              /* "xy" was erased on screen */
}
