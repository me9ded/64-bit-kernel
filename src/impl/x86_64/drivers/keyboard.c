#include <keyboard.h>
#include <cpu.h>
#include <interrupts.h>
#include <kprintf.h>
#include <stdbool.h>

#define PS2_DATA   0x60
#define PS2_STATUS 0x64
#define PS2_STATUS_OUTPUT_FULL 0x01

/* Scancode set 1: a key's release code is its press code with the top bit set. */
#define RELEASED(scancode) ((scancode) & 0x80)
#define KEY_CODE(scancode) ((scancode) & 0x7F)

#define SC_EXTENDED_PREFIX 0xE0
#define SC_LEFT_CTRL   0x1D   /* right Ctrl is E0 1D */
#define SC_LEFT_SHIFT  0x2A
#define SC_RIGHT_SHIFT 0x36
#define SC_CAPS_LOCK   0x3A

/* Characters for each scancode, without and with Shift (0 = no character). */
static const char normal_map[0x59] = {
    [0x01] = 27,   /* Esc */
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-', [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't',
    [0x15] = 'y', [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p',
    [0x1A] = '[', [0x1B] = ']', [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g',
    [0x23] = 'h', [0x24] = 'j', [0x25] = 'k', [0x26] = 'l',
    [0x27] = ';', [0x28] = '\'', [0x29] = '`', [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b',
    [0x31] = 'n', [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/',
    [0x37] = '*', [0x39] = ' ',
    /* keypad (treated as if Num Lock is on) */
    [0x47] = '7', [0x48] = '8', [0x49] = '9', [0x4A] = '-',
    [0x4B] = '4', [0x4C] = '5', [0x4D] = '6', [0x4E] = '+',
    [0x4F] = '1', [0x50] = '2', [0x51] = '3', [0x52] = '0', [0x53] = '.',
};

static const char shifted_map[0x59] = {
    [0x01] = 27,
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')',
    [0x0C] = '_', [0x0D] = '+', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'Q', [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T',
    [0x15] = 'Y', [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P',
    [0x1A] = '{', [0x1B] = '}', [0x1C] = '\n',
    [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F', [0x22] = 'G',
    [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
    [0x27] = ':', [0x28] = '"', [0x29] = '~', [0x2B] = '|',
    [0x2C] = 'Z', [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B',
    [0x31] = 'N', [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
    [0x37] = '*', [0x39] = ' ',
    [0x47] = '7', [0x48] = '8', [0x49] = '9', [0x4A] = '-',
    [0x4B] = '4', [0x4C] = '5', [0x4D] = '6', [0x4E] = '+',
    [0x4F] = '1', [0x50] = '2', [0x51] = '3', [0x52] = '0', [0x53] = '.',
};

/* Ring buffer: the IRQ handler adds keys at `head`, readers take them from `tail`. */
#define BUFFER_SIZE 128
static volatile int buffer[BUFFER_SIZE];
static volatile uint32_t head;
static volatile uint32_t tail;

static bool left_shift;
static bool right_shift;
static bool ctrl_left;
static bool ctrl_right;
static bool caps_lock;
static bool caps_key_down; /* held keys repeat: only toggle on the first press */
static bool extended;      /* the previous byte was the 0xE0 prefix */

static void push_key(int key) {
    uint32_t next = (head + 1) % BUFFER_SIZE;
    if (next == tail) {
        return; /* buffer full: drop the key */
    }
    buffer[head] = key;
    head = next;
}

static void process_extended(uint8_t code, bool released) {
    if (code == SC_LEFT_CTRL) {        /* E0 1D = right Ctrl */
        ctrl_right = !released;
        return;
    }
    if (released) {
        return;
    }

    switch (code) {
        case 0x48: push_key(KEY_UP); break;
        case 0x50: push_key(KEY_DOWN); break;
        case 0x4B: push_key(KEY_LEFT); break;
        case 0x4D: push_key(KEY_RIGHT); break;
        case 0x47: push_key(KEY_HOME); break;
        case 0x4F: push_key(KEY_END); break;
        case 0x53: push_key(KEY_DELETE); break;
        case 0x1C: push_key('\n'); break;   /* keypad Enter */
        case 0x35: push_key('/'); break;    /* keypad / */
        default: break;                     /* other extended keys are ignored */
    }
}

void keyboard_process_scancode(uint8_t scancode) {
    if (scancode == SC_EXTENDED_PREFIX) {
        extended = true;
        return;
    }

    uint8_t code = KEY_CODE(scancode);
    bool released = RELEASED(scancode);

    if (extended) {
        extended = false;
        process_extended(code, released);
        return;
    }

    switch (code) {
        case SC_LEFT_SHIFT:
            left_shift = !released;
            return;
        case SC_RIGHT_SHIFT:
            right_shift = !released;
            return;
        case SC_LEFT_CTRL:
            ctrl_left = !released;
            return;
        case SC_CAPS_LOCK:
            if (!released && !caps_key_down) {
                caps_lock = !caps_lock;
            }
            caps_key_down = !released;
            return;
        default:
            break;
    }

    if (released || code >= sizeof normal_map) {
        return;
    }

    bool shift_held = left_shift || right_shift;
    char character = shift_held ? shifted_map[code] : normal_map[code];
    if (character == 0) {
        return; /* a key without a character (F1-F12, Alt, ...) */
    }

    bool is_letter = (normal_map[code] >= 'a' && normal_map[code] <= 'z');
    if (is_letter && caps_lock) {
        /* Caps Lock flips the case of letters only; Shift+Caps gives lowercase */
        character = shift_held ? normal_map[code] : shifted_map[code];
    }
    if (is_letter && (ctrl_left || ctrl_right)) {
        push_key(KEY_CTRL(character));
        return;
    }

    push_key(character);
}

static void keyboard_irq(struct interrupt_frame* frame) {
    (void) frame;
    keyboard_process_scancode(inb(PS2_DATA));
}

void keyboard_reset(void) {
    head = 0;
    tail = 0;
    left_shift = right_shift = ctrl_left = ctrl_right = false;
    caps_lock = caps_key_down = extended = false;
}

void keyboard_init(void) {
    /* Drop bytes left over from the firmware (bounded, in case there's no controller). */
    for (int i = 0; i < 64 && (inb(PS2_STATUS) & PS2_STATUS_OUTPUT_FULL); i++) {
        inb(PS2_DATA);
    }
    keyboard_reset();
    irq_set_handler(1, keyboard_irq);
}

int keyboard_try_read(void) {
    if (tail == head) {
        return -1;
    }
    int key = buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return key;
}

int keyboard_read_key(void) {
    if (!interrupts_enabled()) {
        panic("keyboard_read_key() called with interrupts disabled: no key could ever arrive");
    }
    for (;;) {
        int key = keyboard_try_read();
        if (key >= 0) {
            return key;
        }
        /* If a key slips in right before hlt, the next timer tick wakes us (10 ms). */
        hlt();
    }
}
