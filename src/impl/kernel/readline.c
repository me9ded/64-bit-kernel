#include <readline.h>
#include <keyboard.h>
#include <kprintf.h>
#include <stdbool.h>

int readline(char* buffer, size_t size) {
    if (size == 0) {
        return -1;
    }

    size_t length = 0;
    buffer[0] = '\0';

    for (;;) {
        int key = keyboard_read_key();

        if (key == '\n') {
            kprintf("\n");
            buffer[length] = '\0';
            return (int) length;
        }

        if (key == '\b') {
            if (length > 0) {
                length--;
                kprintf("\b");
            }
            continue;
        }

        if (key == KEY_CTRL('c')) {
            kprintf("^C\n");
            buffer[0] = '\0';
            return -1;
        }

        bool printable = key >= ' ' && key <= '~';
        if (printable && length + 1 < size) {
            buffer[length++] = (char) key;
            kprintf("%c", key);
        }
        /* anything else (arrows, Tab, Esc, ...) is ignored for now */
    }
}
