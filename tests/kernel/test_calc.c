#include "test.h"
#include <calc.h>

#define CHECK_CALC(expression, expected)                                        \
    do {                                                                        \
        int64_t result_ = 0;                                                    \
        const char* error_ = NULL;                                              \
        if (calc_evaluate(expression, &result_, &error_) != 0) {                \
            test_fail(__FILE__, __LINE__, "\"%s\": error \"%s\", expected %lld", \
                      expression, error_, (long long) (expected));              \
        } else if (result_ != (expected)) {                                     \
            test_fail(__FILE__, __LINE__, "\"%s\": got %ld, expected %lld",      \
                      expression, result_, (long long) (expected));             \
        }                                                                       \
    } while (0)

#define CHECK_CALC_ERROR(expression, expected_error)                            \
    do {                                                                        \
        int64_t result_ = 0;                                                    \
        const char* error_ = NULL;                                              \
        if (calc_evaluate(expression, &result_, &error_) == 0) {                \
            test_fail(__FILE__, __LINE__, "\"%s\": got %ld, expected error \"%s\"", \
                      expression, result_, expected_error);                     \
        } else {                                                                \
            CHECK_EQ_STR(error_, expected_error);                               \
        }                                                                       \
    } while (0)

TEST(calc_basic_operations) {
    CHECK_CALC("1+2", 3);
    CHECK_CALC("10 - 4", 6);
    CHECK_CALC("6 * 7", 42);
    CHECK_CALC("7 / 2", 3);      /* integer division rounds toward zero */
    CHECK_CALC("-7 / 2", -3);
    CHECK_CALC("17 % 5", 2);
    CHECK_CALC("  42  ", 42);
}

TEST(calc_precedence_and_associativity) {
    CHECK_CALC("2 + 3 * 4", 14);
    CHECK_CALC("2 * 3 + 4", 10);
    CHECK_CALC("10 - 4 - 3", 3);     /* left to right: (10 - 4) - 3 */
    CHECK_CALC("100 / 10 / 5", 2);
    CHECK_CALC("1 + 10 % 4 * 2", 5);
}

TEST(calc_parentheses_and_unary) {
    CHECK_CALC("(2 + 3) * 4", 20);
    CHECK_CALC("((((1))))", 1);
    CHECK_CALC("-5 + 2", -3);
    CHECK_CALC("-(2 + 3)", -5);
    CHECK_CALC("--4", 4);
    CHECK_CALC("+7", 7);
    CHECK_CALC("2 * -3", -6);
    CHECK_CALC("9223372036854775807", 9223372036854775807LL);
}

TEST(calc_reports_errors) {
    CHECK_CALC_ERROR("1 / 0", "division by zero");
    CHECK_CALC_ERROR("5 % (2 - 2)", "modulo by zero");
    CHECK_CALC_ERROR("(1 + 2", "missing ')'");
    CHECK_CALC_ERROR("1 + 2)", "unexpected character ')'");
    CHECK_CALC_ERROR("2 $ 3", "unexpected character '$'");
    CHECK_CALC_ERROR("2 3", "unexpected character '3'");
    CHECK_CALC_ERROR("1 +", "unexpected end of expression");
    CHECK_CALC_ERROR("", "empty expression");
    CHECK_CALC_ERROR("   ", "empty expression");
}

TEST(calc_detects_overflow) {
    CHECK_CALC_ERROR("99999999999999999999", "number too large");
    CHECK_CALC_ERROR("9223372036854775807 + 1", "overflow");
    CHECK_CALC_ERROR("-9223372036854775807 - 2", "overflow");
    CHECK_CALC_ERROR("4611686018427387904 * 2", "overflow");
    CHECK_CALC("-9223372036854775807 - 1", -9223372036854775807LL - 1);
    CHECK_CALC_ERROR("(-9223372036854775807 - 1) / -1", "overflow");
}
