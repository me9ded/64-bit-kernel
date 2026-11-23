#include <kprintf.h>
#include <print.h>
#include <serial.h>
#include <cpu.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * The formatter writes through a "sink", so the same code can print to the
 * screen + serial port (kprintf) or into a memory buffer (ksnprintf).
 */
struct sink {
    void (*put)(struct sink* sink, char character);
    char* buffer;   /* only used by the buffer sink */
    size_t size;
    int count;      /* characters produced so far */
};

static void put(struct sink* sink, char character) {
    sink->put(sink, character);
    sink->count++;
}

static bool serial_mirror = true;

void kprintf_set_serial_mirror(bool enabled) {
    serial_mirror = enabled;
}

static void console_put(struct sink* sink, char character) {
    (void) sink;
    print_char(character);
    if (serial_mirror) {
        serial_write_char(character);
    }
}

static void buffer_put(struct sink* sink, char character) {
    /* keep one byte for the terminating NUL */
    if (sink->size > 0 && (size_t) sink->count < sink->size - 1) {
        sink->buffer[sink->count] = character;
    }
}

static void pad(struct sink* sink, int count, char with) {
    for (; count > 0; count--) {
        put(sink, with);
    }
}

static void put_string(struct sink* sink, const char* string, int width, bool left_align) {
    if (string == NULL) {
        string = "(null)";
    }
    int length = 0;
    while (string[length] != '\0') {
        length++;
    }

    if (!left_align) {
        pad(sink, width - length, ' ');
    }
    for (int i = 0; i < length; i++) {
        put(sink, string[i]);
    }
    if (left_align) {
        pad(sink, width - length, ' ');
    }
}

static void put_number(struct sink* sink, unsigned long long value, bool negative, unsigned base, bool uppercase, int width, bool zero_pad, bool left_align) {
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char buffer[24]; /* enough for 2^64 in decimal (20 digits) */
    int length = 0;

    do {
        buffer[length++] = digits[value % base];
        value /= base;
    } while (value != 0);

    int total = length + (negative ? 1 : 0);

    if (left_align) {
        zero_pad = false;
    }
    if (!left_align && !zero_pad) {
        pad(sink, width - total, ' ');
    }
    if (negative) {
        put(sink, '-');
    }
    if (zero_pad) {
        pad(sink, width - total, '0');
    }
    while (length > 0) {
        put(sink, buffer[--length]);
    }
    if (left_align) {
        pad(sink, width - total, ' ');
    }
}

static void format_to(struct sink* sink, const char* format, va_list args) {
    for (const char* p = format; *p != '\0'; p++) {
        if (*p != '%') {
            put(sink, *p);
            continue;
        }
        p++;

        /* flags */
        bool left_align = false;
        bool zero_pad = false;
        for (;; p++) {
            if (*p == '-') {
                left_align = true;
            } else if (*p == '0') {
                zero_pad = true;
            } else {
                break;
            }
        }

        /* width */
        int width = 0;
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }

        /* length: 0 = int, 1 = long, 2 = long long, 3 = size_t */
        int length = 0;
        if (*p == 'l') {
            length = 1;
            p++;
            if (*p == 'l') {
                length = 2;
                p++;
            }
        } else if (*p == 'z') {
            length = 3;
            p++;
        }

        switch (*p) {
            case 'c':
                pad(sink, left_align ? 0 : width - 1, ' ');
                put(sink, (char) va_arg(args, int));
                pad(sink, left_align ? width - 1 : 0, ' ');
                break;

            case 's':
                put_string(sink, va_arg(args, const char*), width, left_align);
                break;

            case 'd':
            case 'i': {
                long long value;
                if (length == 1) value = va_arg(args, long);
                else if (length == 2) value = va_arg(args, long long);
                else if (length == 3) value = (long long) va_arg(args, size_t);
                else value = va_arg(args, int);

                bool negative = value < 0;
                /* negate in unsigned arithmetic so LLONG_MIN doesn't overflow */
                unsigned long long magnitude = negative ? 0ULL - (unsigned long long) value : (unsigned long long) value;
                put_number(sink, magnitude, negative, 10, false, width, zero_pad, left_align);
                break;
            }

            case 'u':
            case 'x':
            case 'X': {
                unsigned long long value;
                if (length == 1) value = va_arg(args, unsigned long);
                else if (length == 2) value = va_arg(args, unsigned long long);
                else if (length == 3) value = va_arg(args, size_t);
                else value = va_arg(args, unsigned int);

                unsigned base = (*p == 'u') ? 10 : 16;
                put_number(sink, value, false, base, *p == 'X', width, zero_pad, left_align);
                break;
            }

            case 'p': {
                uintptr_t value = (uintptr_t) va_arg(args, void*);
                put(sink, '0');
                put(sink, 'x');
                put_number(sink, value, false, 16, false, 16, true, false);
                break;
            }

            case '%':
                put(sink, '%');
                break;

            case '\0':
                /* format string ended right after '%' */
                return;

            default:
                /* unknown specifier: print it as-is so the mistake is visible */
                put(sink, '%');
                put(sink, *p);
                break;
        }
    }
}

int vkprintf(const char* format, va_list args) {
    struct sink sink = { .put = console_put };
    format_to(&sink, format, args);
    return sink.count;
}

int kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = vkprintf(format, args);
    va_end(args);
    return written;
}

int kvsnprintf(char* buffer, size_t size, const char* format, va_list args) {
    struct sink sink = { .put = buffer_put, .buffer = buffer, .size = size };
    format_to(&sink, format, args);
    if (size > 0) {
        size_t end = (size_t) sink.count < size - 1 ? (size_t) sink.count : size - 1;
        buffer[end] = '\0';
    }
    return sink.count;
}

int ksnprintf(char* buffer, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = kvsnprintf(buffer, size, format, args);
    va_end(args);
    return written;
}

void panic(const char* format, ...) {
    serial_mirror = true;
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_RED);
    kprintf("\nKERNEL PANIC: ");

    va_list args;
    va_start(args, format);
    vkprintf(format, args);
    va_end(args);

    kprintf("\n");

    if (panic_hook) {
        panic_hook();
    }
    halt_forever();
}
