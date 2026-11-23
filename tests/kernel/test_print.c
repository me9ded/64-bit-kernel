#include "test.h"
#include <kprintf.h>
#include <print.h>

#define YELLOW_ON_BLACK 0x0e
#define WHITE_ON_RED    0x4f

TEST(print_clear_blanks_screen_and_resets_cursor) {
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("junk\njunk");
    print_clear();

    for (size_t row = 0; row < 25; row++) {
        for (size_t col = 0; col < 80; col++) {
            if (vga_char(row, col) != ' ' || vga_color(row, col) != YELLOW_ON_BLACK) {
                test_fail(__FILE__, __LINE__, "cell (%zu,%zu) not blank", row, col);
                return;
            }
        }
    }

    print_char('A');
    CHECK_EQ_INT(vga_char(0, 0), 'A');
}

TEST(print_char_writes_with_current_color) {
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_clear();
    CHECK_EQ_INT(print_char('X'), 0);
    CHECK_EQ_INT(vga_char(0, 0), 'X');
    CHECK_EQ_INT(vga_color(0, 0), YELLOW_ON_BLACK);
}

TEST(print_newline_moves_to_next_row) {
    print_clear();
    CHECK_EQ_INT(print_str("ab\ncd"), 0);
    CHECK_EQ_INT(vga_char(0, 0), 'a');
    CHECK_EQ_INT(vga_char(0, 1), 'b');
    CHECK_EQ_INT(vga_char(1, 0), 'c');
    CHECK_EQ_INT(vga_char(1, 1), 'd');
}

TEST(print_wraps_after_80_columns) {
    print_clear();
    for (int i = 0; i < 80; i++) {
        print_char('x');
    }
    print_char('y');
    CHECK_EQ_INT(vga_char(0, 79), 'x');
    CHECK_EQ_INT(vga_char(1, 0), 'y');
}

TEST(print_scrolls_and_clears_bottom_row) {
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_clear();
    char line[8];
    for (int i = 0; i < 26; i++) {
        ksnprintf(line, sizeof line, "L%02d\n", i);
        print_str(line);
    }

    /* 26 lines, each ending in '\n', on a 25-row screen: L00 and L01 scrolled away */
    CHECK_EQ_INT(vga_char(0, 1), '0');
    CHECK_EQ_INT(vga_char(0, 2), '2');
    CHECK_EQ_INT(vga_char(23, 1), '2');
    CHECK_EQ_INT(vga_char(23, 2), '5');
    for (size_t col = 0; col < 80; col++) {
        if (vga_char(24, col) != ' ' || vga_color(24, col) != YELLOW_ON_BLACK) {
            test_fail(__FILE__, __LINE__, "bottom row not blank at column %zu", col);
            return;
        }
    }
}

TEST(print_set_color_accepts_valid_and_rejects_invalid) {
    CHECK_EQ_INT(print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_RED), 0);
    print_clear();
    print_char('A');
    CHECK_EQ_INT(vga_color(0, 0), WHITE_ON_RED);

    CHECK_EQ_INT(print_set_color(16, PRINT_COLOR_BLACK), -1);
    CHECK_EQ_INT(print_set_color(PRINT_COLOR_BLACK, 16), -1);
    print_char('B');
    CHECK_EQ_INT(vga_color(0, 1), WHITE_ON_RED); /* color unchanged after rejection */
}

TEST(backspace_erases_previous_character) {
    print_clear();
    print_str("ab\b");
    CHECK_EQ_INT(vga_char(0, 0), 'a');
    CHECK_EQ_INT(vga_char(0, 1), ' ');
    print_char('c');
    CHECK_EQ_INT(vga_char(0, 1), 'c');
}

TEST(backspace_at_line_start_goes_to_previous_row) {
    print_clear();
    for (int i = 0; i < 80; i++) {
        print_char('x');
    }
    print_str("\n\b");           /* back from (1,0) to the last cell of row 0 */
    CHECK_EQ_INT(vga_char(0, 79), ' ');
    print_char('y');
    CHECK_EQ_INT(vga_char(0, 79), 'y');
}

TEST(backspace_at_top_left_does_nothing) {
    print_clear();
    print_str("\b\bz");
    CHECK_EQ_INT(vga_char(0, 0), 'z');
}

TEST(tab_moves_to_next_multiple_of_8) {
    print_clear();
    print_str("a\tb\tc");
    CHECK_EQ_INT(vga_char(0, 0), 'a');
    CHECK_EQ_INT(vga_char(0, 1), ' ');
    CHECK_EQ_INT(vga_char(0, 8), 'b');
    CHECK_EQ_INT(vga_char(0, 16), 'c');

    print_clear();
    print_str("12345678\tX");    /* already on a tab stop: moves a full 8 */
    CHECK_EQ_INT(vga_char(0, 16), 'X');
}

TEST(hardware_cursor_follows_text) {
    print_clear();
    CHECK_EQ_INT(vga_cursor(), 0);
    print_str("abc");
    CHECK_EQ_INT(vga_cursor(), 3);
    print_str("\n");
    CHECK_EQ_INT(vga_cursor(), 80);
    print_str("x\b");
    CHECK_EQ_INT(vga_cursor(), 80);
    print_str("\t");
    CHECK_EQ_INT(vga_cursor(), 88);
}
