#pragma once

#include <stddef.h>

/*
 * Read one line from the keyboard, echoing it on screen and serial.
 *
 *   printable keys  added to the line (ignored once `size - 1` characters are stored)
 *   Backspace       removes the last character
 *   Enter           finishes the line
 *   Ctrl+C          cancels: prints "^C" and returns -1
 *
 * On success `buffer` holds the NUL-terminated line (without '\n') and the
 * return value is its length. Needs keyboard_init() and interrupts enabled.
 */
int readline(char* buffer, size_t size);
