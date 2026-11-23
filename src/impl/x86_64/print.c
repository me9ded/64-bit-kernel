#include <print.h>
#include <cpu.h>

static const size_t NUM_COLS = 80;
static const size_t NUM_ROWS = 25;
static const size_t TAB_WIDTH = 8;

/* VGA CRT controller: select a register on 0x3D4, then read/write it on 0x3D5 */
#define VGA_CRTC_INDEX        0x3D4
#define VGA_CRTC_DATA         0x3D5
#define VGA_CURSOR_START      0x0A
#define VGA_CURSOR_END        0x0B
#define VGA_CURSOR_LOCATION_H 0x0E
#define VGA_CURSOR_LOCATION_L 0x0F

struct Char {
    uint8_t character;
    uint8_t color;
};

static struct Char* const buffer = (struct Char*) 0xb8000;
static size_t col = 0;
static size_t row = 0;
static uint8_t color = PRINT_COLOR_GREEN | PRINT_COLOR_CYAN << 4;

/* Move the blinking hardware cursor to where the next character will go. */
static void update_cursor(void) {
    size_t position = row * NUM_COLS + col; /* col == 80 means "start of next row" */
    if (position >= NUM_ROWS * NUM_COLS) {
        position = NUM_ROWS * NUM_COLS - 1;
    }
    outb(VGA_CRTC_INDEX, VGA_CURSOR_LOCATION_L);
    outb(VGA_CRTC_DATA, (uint8_t) (position & 0xFF));
    outb(VGA_CRTC_INDEX, VGA_CURSOR_LOCATION_H);
    outb(VGA_CRTC_DATA, (uint8_t) ((position >> 8) & 0xFF));
}

void print_enable_cursor(void) {
    /* underline cursor: scanlines 14-15 of the 16-pixel-high character cell */
    outb(VGA_CRTC_INDEX, VGA_CURSOR_START);
    outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & 0xC0) | 14);
    outb(VGA_CRTC_INDEX, VGA_CURSOR_END);
    outb(VGA_CRTC_DATA, (inb(VGA_CRTC_DATA) & 0xE0) | 15);
    update_cursor();
}

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
    row = 0;
    col = 0;
    update_cursor();
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

static void print_backspace(void) {
    if (col == 0) {
        if (row == 0) {
            return; /* nothing before the top-left corner */
        }
        row--;
        col = NUM_COLS;
    }
    col--;

    buffer[col + NUM_COLS * row] = (struct Char) {
        .character = ' ',
        .color = color,
    };
}

static void put_visible(char character) {
    if (col >= NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        .character = (uint8_t) character,
        .color = color,
    };

    col++;
}

int print_char(char character) {
    switch (character) {
        case '\n':
            print_newline();
            break;
        case '\b':
            print_backspace();
            break;
        case '\t':
            do {
                put_visible(' ');
            } while (col % TAB_WIDTH != 0);
            break;
        default:
            put_visible(character);
            break;
    }

    update_cursor();
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
