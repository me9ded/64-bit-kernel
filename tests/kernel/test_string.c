#include "test.h"

TEST(memset_fills_bytes) {
    char buffer[8] = "abcdefg";
    CHECK(memset(buffer, 'z', 3) == buffer);
    CHECK_EQ_STR(buffer, "zzzdefg");
}

TEST(memcpy_copies_bytes) {
    char buffer[8] = "abcdefg";
    CHECK(memcpy(buffer, "XYZ", 3) == buffer);
    CHECK_EQ_STR(buffer, "XYZdefg");
}

TEST(memmove_handles_overlap_forward) {
    char buffer[16] = "abcdefgh";
    memmove(buffer + 2, buffer, 6);
    CHECK_EQ_STR(buffer, "ababcdef");
}

TEST(memmove_handles_overlap_backward) {
    char buffer[16] = "abcdefgh";
    memmove(buffer, buffer + 2, 6);
    CHECK_EQ_STR(buffer, "cdefghgh");
}

TEST(memcmp_orders_bytes) {
    CHECK_EQ_INT(memcmp("abc", "abc", 3), 0);
    CHECK(memcmp("abc", "abd", 3) < 0);
    CHECK(memcmp("abd", "abc", 3) > 0);
    CHECK(memcmp("\x01", "\xff", 1) < 0); /* bytes compare as unsigned */
    CHECK_EQ_INT(memcmp("abX", "abY", 2), 0);
}

TEST(strlen_counts_characters) {
    CHECK_EQ_INT(strlen(""), 0);
    CHECK_EQ_INT(strlen("hello"), 5);
}

TEST(strcmp_orders_strings) {
    CHECK_EQ_INT(strcmp("abc", "abc"), 0);
    CHECK(strcmp("abc", "abd") < 0);
    CHECK(strcmp("b", "a") > 0);
    CHECK(strcmp("ab", "abc") < 0);
    CHECK(strcmp("abc", "ab") > 0);
    CHECK_EQ_INT(strcmp("", ""), 0);
}

TEST(strncmp_stops_after_n) {
    CHECK_EQ_INT(strncmp("abcX", "abcY", 3), 0);
    CHECK(strncmp("abcX", "abcY", 4) < 0);
    CHECK_EQ_INT(strncmp("ab", "ab", 10), 0);
    CHECK(strncmp("ab", "abc", 10) < 0);
    CHECK_EQ_INT(strncmp("x", "y", 0), 0);
}

TEST(strstr_finds_substrings) {
    const char* text = "hello world";
    CHECK(strstr(text, "world") == text + 6);
    CHECK(strstr(text, "hello") == text);
    CHECK(strstr(text, "o w") == text + 4);
    CHECK(strstr(text, "") == text);
    CHECK(strstr(text, "worlds") == NULL);
    CHECK(strstr(text, "xyz") == NULL);
    CHECK(strstr("", "a") == NULL);
}
