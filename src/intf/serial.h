#pragma once

/* Serial port COM1 (0x3F8). In QEMU, run with `-serial stdio` to see it in your terminal. */

/* Initialise COM1 at 38400 baud, 8N1. Returns 0 on success, -1 if no working UART was found. */
int serial_init(void);

/* Write to COM1. Does nothing if serial_init() failed or wasn't called. '\n' is sent as "\r\n". */
void serial_write_char(char character);
void serial_write_str(const char* string);
