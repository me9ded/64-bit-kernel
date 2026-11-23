#pragma once

#include <stdint.h>

/*
 * PS/2 keyboard (IRQ 1), US QWERTY layout.
 *
 * Keys are returned as ints:
 *   - ASCII for normal characters, including '\n' (Enter), '\b' (Backspace),
 *     '\t' (Tab) and 27 (Esc)
 *   - control codes for Ctrl+letter: KEY_CTRL('c') == 3
 *   - KEY_* values below for keys without an ASCII code
 */
enum {
    KEY_UP = 0x100,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
};

#define KEY_CTRL(letter) ((letter) & 0x1F)

/* Discard stale input and start receiving keys on IRQ 1 (needs interrupts_init()). */
void keyboard_init(void);

/* Return the next key, or -1 if none is waiting. */
int keyboard_try_read(void);

/* Wait for the next key (the CPU sleeps in between). Needs interrupts enabled. */
int keyboard_read_key(void);

/*
 * Feed one raw scancode to the driver. Called by the IRQ handler; tests call
 * it directly to simulate key presses.
 */
void keyboard_process_scancode(uint8_t scancode);

/* Forget queued keys and modifier state (Shift/Ctrl/Caps Lock). */
void keyboard_reset(void);
