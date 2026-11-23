#pragma once

#include <stdint.h>

/*
 * Evaluate an integer expression such as "(2 + 3) * -4 % 7".
 *
 * Supports + - * / % (C semantics: / rounds toward zero), parentheses,
 * unary + and -, and decimal numbers. Spaces are ignored.
 *
 * Returns 0 and sets *result on success. Returns -1 and sets *error to a
 * message on failure (division by zero, overflow, syntax errors).
 */
int calc_evaluate(const char* expression, int64_t* result, const char** error);
