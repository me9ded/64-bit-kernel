#include <print.h>

static const size_t NUM_COLS = 80;
static const size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

static struct Char* const buffer = (struct Char*) 0xb8000;
static size_t col = 0;
static size_t row = 0;
static uint8_t color = PRINT_COLOR_GREEN | PRINT_COLOR_CYAN << 4;

static void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        .character = ' ',
        .color = color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear(void) {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

static void print_newline(void) {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }

    clear_row(NUM_ROWS - 1);
}

int print_char(char character) {
    if (character == '\n') {
        print_newline();
        return 0;
    }

    if (col >= NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        .character = (uint8_t) character,
        .color = color,
    };

    col++;
    return 0;
}

int print_str(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }

    return 0;
}

int print_set_color(uint8_t foreground, uint8_t background) {
    if (foreground > 15 || background > 15) {
        return -1;
    }

    color = foreground | (background << 4);
    return 0;
}
