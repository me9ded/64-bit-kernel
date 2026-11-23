/*
 * Recursive-descent parser: one function per grammar rule. Precedence falls
 * out of the structure: an expression is a sum of terms, and a term is a
 * product of factors, so * / % bind tighter than + -.
 *
 *   expression := term (('+' | '-') term)*
 *   term       := factor (('*' | '/' | '%') factor)*
 *   factor     := ('+' | '-') factor | number | '(' expression ')'
 */
#include <calc.h>
#include <kprintf.h>
#include <stdbool.h>

struct parser {
    const char* position;
    const char* error;
};

static char error_buffer[48];

static bool fail(struct parser* parser, const char* message) {
    if (parser->error == NULL) {
        parser->error = message;
    }
    return false;
}

static bool fail_unexpected(struct parser* parser) {
    if (*parser->position == '\0') {
        return fail(parser, "unexpected end of expression");
    }
    ksnprintf(error_buffer, sizeof error_buffer, "unexpected character '%c'", *parser->position);
    return fail(parser, error_buffer);
}

static void skip_spaces(struct parser* parser) {
    while (*parser->position == ' ' || *parser->position == '\t') {
        parser->position++;
    }
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool parse_expression(struct parser* parser, int64_t* value);

static bool parse_number(struct parser* parser, int64_t* value) {
    int64_t number = 0;
    while (is_digit(*parser->position)) {
        int digit = *parser->position - '0';
        if (__builtin_mul_overflow(number, 10, &number) ||
            __builtin_add_overflow(number, digit, &number)) {
            return fail(parser, "number too large");
        }
        parser->position++;
    }
    *value = number;
    return true;
}

static bool parse_factor(struct parser* parser, int64_t* value) {
    skip_spaces(parser);
    char c = *parser->position;

    if (c == '-' || c == '+') {
        parser->position++;
        int64_t operand;
        if (!parse_factor(parser, &operand)) {
            return false;
        }
        if (c == '-' && __builtin_sub_overflow((int64_t) 0, operand, value)) {
            return fail(parser, "overflow");
        }
        if (c == '+') {
            *value = operand;
        }
        return true;
    }

    if (c == '(') {
        parser->position++;
        if (!parse_expression(parser, value)) {
            return false;
        }
        skip_spaces(parser);
        if (*parser->position != ')') {
            return fail(parser, "missing ')'");
        }
        parser->position++;
        return true;
    }

    if (is_digit(c)) {
        return parse_number(parser, value);
    }

    return fail_unexpected(parser);
}

static bool parse_term(struct parser* parser, int64_t* value) {
    if (!parse_factor(parser, value)) {
        return false;
    }

    for (;;) {
        skip_spaces(parser);
        char op = *parser->position;
        if (op != '*' && op != '/' && op != '%') {
            return true;
        }
        parser->position++;

        int64_t right;
        if (!parse_factor(parser, &right)) {
            return false;
        }

        if (op == '*') {
            if (__builtin_mul_overflow(*value, right, value)) {
                return fail(parser, "overflow");
            }
            continue;
        }
        if (right == 0) {
            return fail(parser, op == '/' ? "division by zero" : "modulo by zero");
        }
        if (*value == INT64_MIN && right == -1) {
            return fail(parser, "overflow"); /* the one case where / and % overflow */
        }
        *value = (op == '/') ? *value / right : *value % right;
    }
}

static bool parse_expression(struct parser* parser, int64_t* value) {
    if (!parse_term(parser, value)) {
        return false;
    }

    for (;;) {
        skip_spaces(parser);
        char op = *parser->position;
        if (op != '+' && op != '-') {
            return true;
        }
        parser->position++;

        int64_t right;
        if (!parse_term(parser, &right)) {
            return false;
        }
        bool overflow = (op == '+') ? __builtin_add_overflow(*value, right, value)
                                    : __builtin_sub_overflow(*value, right, value);
        if (overflow) {
            return fail(parser, "overflow");
        }
    }
}

int calc_evaluate(const char* expression, int64_t* result, const char** error) {
    struct parser parser = { .position = expression, .error = NULL };

    skip_spaces(&parser);
    if (*parser.position == '\0') {
        *error = "empty expression";
        return -1;
    }

    int64_t value;
    if (!parse_expression(&parser, &value)) {
        *error = parser.error;
        return -1;
    }

    skip_spaces(&parser);
    if (*parser.position != '\0') {
        fail_unexpected(&parser); /* e.g. "2 3" or "1 + 2)" */
        *error = parser.error;
        return -1;
    }

    *result = value;
    return 0;
}
