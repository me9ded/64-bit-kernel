#include <readline.h>
#include <keyboard.h>
#include <kprintf.h>
#include <stdbool.h>
#include <string.h>

#define HISTORY_SIZE 16
#define HISTORY_LINE 128

/* history[0] is the oldest line, history[history_count - 1] the newest. */
static char history[HISTORY_SIZE][HISTORY_LINE];
static int history_count;

void readline_clear_history(void) {
    history_count = 0;
}

static void history_add(const char* line) {
    if (line[0] == '\0') {
        return;
    }
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return; /* don't store the same command twice in a row */
    }
    if (history_count == HISTORY_SIZE) {
        memmove(history[0], history[1], sizeof history[0] * (HISTORY_SIZE - 1));
        history_count--;
    }

    size_t length = strlen(line);
    if (length >= HISTORY_LINE) {
        length = HISTORY_LINE - 1;
    }
    memcpy(history[history_count], line, length);
    history[history_count][length] = '\0';
    history_count++;
}

/* Erase the line on screen and replace it (and the buffer) with `text`. */
static void replace_line(char* buffer, size_t size, size_t* length, const char* text) {
    for (; *length > 0; (*length)--) {
        kprintf("\b");
    }
    for (size_t i = 0; text[i] != '\0' && *length + 1 < size; i++) {
        buffer[(*length)++] = text[i];
        kprintf("%c", text[i]);
    }
}

int readline(char* buffer, size_t size) {
    if (size == 0) {
        return -1;
    }

    size_t length = 0;
    int history_position = history_count; /* == history_count: the new line being typed */
    buffer[0] = '\0';

    for (;;) {
        int key = keyboard_read_key();

        if (key == '\n') {
            kprintf("\n");
            buffer[length] = '\0';
            history_add(buffer);
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

        if (key == KEY_UP && history_position > 0) {
            history_position--;
            replace_line(buffer, size, &length, history[history_position]);
            continue;
        }

        if (key == KEY_DOWN && history_position < history_count) {
            history_position++;
            replace_line(buffer, size, &length,
                         history_position == history_count ? "" : history[history_position]);
            continue;
        }

        bool printable = key >= ' ' && key <= '~';
        if (printable && length + 1 < size) {
            buffer[length++] = (char) key;
            kprintf("%c", key);
        }
        /* anything else (Left/Right, Tab, Esc, ...) is ignored for now */
    }
}
