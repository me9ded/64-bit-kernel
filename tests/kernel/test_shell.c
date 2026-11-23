#include "test.h"
#include <kprintf.h>
#include <print.h>
#include <shell.h>
#include <timer.h>

/* Clear the screen, run `command`, and return shell_execute's result. */
static int run(const char* command) {
    char line[128];
    size_t length = strlen(command);
    memcpy(line, command, length + 1);
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_clear();
    return shell_execute(line);
}

static void row_text(size_t row, char* out) {
    vga_row_text(row, out, 81);
}

TEST(shell_split_handles_extra_spaces) {
    char line[] = "  echo   a\tb  ";
    char* argv[4];
    CHECK_EQ_INT(shell_split(line, argv, 4), 3);
    CHECK_EQ_STR(argv[0], "echo");
    CHECK_EQ_STR(argv[1], "a");
    CHECK_EQ_STR(argv[2], "b");
}

TEST(shell_split_empty_and_too_many) {
    char blank[] = "   ";
    char* argv[2];
    CHECK_EQ_INT(shell_split(blank, argv, 2), 0);
    char many[] = "a b c";
    CHECK_EQ_INT(shell_split(many, argv, 2), -1);
}

TEST(shell_empty_line_does_nothing) {
    CHECK_EQ_INT(run(""), 0);
    CHECK_EQ_INT(run("    "), 0);
    CHECK_EQ_INT(vga_cursor(), 0);
}

TEST(shell_unknown_command) {
    CHECK_EQ_INT(run("foo bar"), -1);
    CHECK(vga_contains("unknown command: foo"));
}

TEST(shell_too_many_arguments) {
    CHECK_EQ_INT(run("echo 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17"), -1);
    CHECK(vga_contains("too many arguments"));
}

TEST(shell_help_lists_every_command) {
    static const char* const names[] = {
        "help", "clear", "echo", "calc", "color", "uptime", "sleep", "date", "reboot", "shutdown",
    };
    CHECK_EQ_INT(run("help"), 0);
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        char usage[24];
        ksnprintf(usage, sizeof usage, "  %s", names[i]);
        if (!vga_contains(usage)) {
            test_fail(__FILE__, __LINE__, "help doesn't list '%s'", names[i]);
        }
    }
}

TEST(shell_echo) {
    char row[81];
    CHECK_EQ_INT(run("echo hello   world"), 0);
    row_text(0, row);
    CHECK_EQ_STR(row, "hello world");
    CHECK_EQ_INT(run("echo"), 0);
    row_text(0, row);
    CHECK_EQ_STR(row, "");
}

TEST(shell_clear) {
    print_clear();
    print_str("junk");
    char line[] = "clear";
    CHECK_EQ_INT(shell_execute(line), 0);
    char row[81];
    row_text(0, row);
    CHECK_EQ_STR(row, "");
    CHECK_EQ_INT(vga_cursor(), 0);
}

TEST(shell_calc) {
    char row[81];
    CHECK_EQ_INT(run("calc (2 + 3) * 4"), 0);
    row_text(0, row);
    CHECK_EQ_STR(row, "= 20");
    CHECK_EQ_INT(run("calc 7/0"), 1);
    row_text(0, row);
    CHECK_EQ_STR(row, "error: division by zero");
    CHECK_EQ_INT(run("calc"), 1);
    CHECK(vga_contains("usage: calc"));
}

TEST(shell_color) {
    CHECK_EQ_INT(run("color white red"), 0);
    print_char('A');
    CHECK_EQ_INT(vga_color(0, 0), 0x4f);

    CHECK_EQ_INT(shell_execute((char[]) { "color yellow" }), 0); /* background stays red */
    CHECK_EQ_INT(print_get_color(), 0x4e);

    CHECK_EQ_INT(shell_execute((char[]) { "color 15 1" }), 0);   /* by number */
    CHECK_EQ_INT(print_get_color(), 0x1f);

    CHECK_EQ_INT(run("color purple"), 1);
    CHECK(vga_contains("unknown color: purple"));
    CHECK_EQ_INT(run("color 16"), 1);

    CHECK_EQ_INT(run("color"), 0);                             /* lists the colors */
    CHECK(vga_contains("light-blue"));
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
}

TEST(shell_sleep) {
    uint64_t start = timer_ticks();
    CHECK_EQ_INT(run("sleep 0"), 0);
    CHECK(timer_ticks() - start < 5);
    CHECK_EQ_INT(run("sleep abc"), 1);
    CHECK_EQ_INT(run("sleep"), 1);
    CHECK_EQ_INT(run("sleep 99999"), 1);
}

TEST(shell_uptime_format) {
    char row[81];
    CHECK_EQ_INT(run("uptime"), 0);
    row_text(0, row);
    /* "up HH:MM:SS (N ticks)" */
    CHECK_EQ_INT(strncmp(row, "up ", 3), 0);
    CHECK_EQ_INT(row[5], ':');
    CHECK_EQ_INT(row[8], ':');
    CHECK(strstr(row, " ticks)") != NULL);
}

TEST(shell_date_format) {
    char row[81];
    CHECK_EQ_INT(run("date"), 0);
    row_text(0, row);
    /* "YYYY-MM-DD HH:MM:SS UTC" */
    CHECK_EQ_INT(strlen(row), 23);
    CHECK_EQ_INT(row[4], '-');
    CHECK_EQ_INT(row[7], '-');
    CHECK_EQ_INT(row[13], ':');
    CHECK_EQ_STR(row + 19, " UTC");
}
