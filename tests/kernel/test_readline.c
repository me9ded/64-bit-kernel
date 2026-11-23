#include "test.h"
#include <keyboard.h>
#include <print.h>
#include <readline.h>

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
