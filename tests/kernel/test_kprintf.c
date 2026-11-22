#include "test.h"
#include <kprintf.h>

#define CHECK_FORMAT(expected, ...)                                             \
    do {                                                                        \
        char buffer_[128];                                                      \
        ksnprintf(buffer_, sizeof buffer_, __VA_ARGS__);                        \
        CHECK_EQ_STR(buffer_, expected);                                        \
    } while (0)

TEST(kprintf_integers) {
    CHECK_FORMAT("-42", "%d", -42);
    CHECK_FORMAT("7", "%i", 7);
    CHECK_FORMAT("0", "%d", 0);
    CHECK_FORMAT("3000000000", "%u", 3000000000u);
    CHECK_FORMAT("ff", "%x", 255);
    CHECK_FORMAT("BEEF", "%X", 0xBEEF);
}

TEST(kprintf_width_and_flags) {
    CHECK_FORMAT("   42", "%5d", 42);
    CHECK_FORMAT("42   ", "%-5d", 42);
    CHECK_FORMAT("-0042", "%05d", -42);
    CHECK_FORMAT("  -42", "%5d", -42);
    CHECK_FORMAT("000000ab", "%08x", 0xab);
    CHECK_FORMAT("123456", "%3d", 123456); /* width is a minimum, never truncates */
}

TEST(kprintf_length_modifiers) {
    CHECK_FORMAT("-1234567890123", "%ld", -1234567890123L);
    CHECK_FORMAT("-9223372036854775808", "%lld", -9223372036854775807LL - 1);
    CHECK_FORMAT("18446744073709551615", "%llu", 18446744073709551615ULL);
    CHECK_FORMAT("deadbeefcafe", "%llx", 0xdeadbeefcafeULL);
    CHECK_FORMAT("4096", "%zu", (size_t) 4096);
}

TEST(kprintf_chars_and_strings) {
    const char* volatile null_string = NULL; /* volatile: hide the NULL from gcc's format checker */
    CHECK_FORMAT("K", "%c", 'K');
    CHECK_FORMAT("  a|b  ", "%3c|%-3c", 'a', 'b');
    CHECK_FORMAT("hi", "%s", "hi");
    CHECK_FORMAT("   right|left    ", "%8s|%-8s", "right", "left");
    CHECK_FORMAT("(null)", "%s", null_string);
}

TEST(kprintf_pointers_and_percent) {
    CHECK_FORMAT("0x00000000000b8000", "%p", (void*) 0xb8000);
    CHECK_FORMAT("100%", "%d%%", 100);
}

TEST(kprintf_bad_formats_are_visible) {
    /* non-literal formats so gcc's compile-time checker doesn't reject them */
    const char* unknown = "[%q]";
    const char* trailing = "abc%";
    CHECK_FORMAT("[%q]", unknown, 0);
    CHECK_FORMAT("abc", trailing, 0);
}

TEST(ksnprintf_truncates_and_returns_full_length) {
    char buffer[4] = "xxx";
    CHECK_EQ_INT(ksnprintf(buffer, sizeof buffer, "%s", "hello"), 5);
    CHECK_EQ_STR(buffer, "hel");

    char untouched[4] = "xyz";
    CHECK_EQ_INT(ksnprintf(untouched, 0, "%s", "hello"), 5);
    CHECK_EQ_STR(untouched, "xyz");

    char exact[6];
    CHECK_EQ_INT(ksnprintf(exact, sizeof exact, "%d", 12345), 5);
    CHECK_EQ_STR(exact, "12345");
}
