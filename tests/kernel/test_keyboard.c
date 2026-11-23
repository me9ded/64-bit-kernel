#include "test.h"
#include <keyboard.h>

/* Scancode set 1 press codes (release = press | 0x80) */
#define SC_A          0x1E
#define SC_B          0x30
#define SC_C          0x2E
#define SC_1          0x02
#define SC_SLASH      0x35
#define SC_ENTER      0x1C
#define SC_BACKSPACE  0x0E
#define SC_SPACE      0x39
#define SC_F1         0x3B
#define SC_LSHIFT     0x2A
#define SC_RSHIFT     0x36
#define SC_CTRL       0x1D
#define SC_CAPS       0x3A
#define SC_EXTENDED   0xE0
#define SC_ARROW_UP   0x48
#define SC_ARROW_LEFT 0x4B
#define RELEASE(sc)   ((sc) | 0x80)

static void tap(uint8_t scancode) {
    keyboard_process_scancode(scancode);
    keyboard_process_scancode(RELEASE(scancode));
}

TEST(keyboard_plain_keys) {
    keyboard_reset();
    tap(SC_A);
    tap(SC_1);
    tap(SC_SPACE);
    tap(SC_ENTER);
    tap(SC_BACKSPACE);
    CHECK_EQ_INT(keyboard_try_read(), 'a');
    CHECK_EQ_INT(keyboard_try_read(), '1');
    CHECK_EQ_INT(keyboard_try_read(), ' ');
    CHECK_EQ_INT(keyboard_try_read(), '\n');
    CHECK_EQ_INT(keyboard_try_read(), '\b');
    CHECK_EQ_INT(keyboard_try_read(), -1);
}

TEST(keyboard_releases_and_unmapped_keys_produce_nothing) {
    keyboard_reset();
    keyboard_process_scancode(RELEASE(SC_A));
    tap(SC_F1);
    CHECK_EQ_INT(keyboard_try_read(), -1);
}

TEST(keyboard_shift) {
    keyboard_reset();
    keyboard_process_scancode(SC_LSHIFT);
    tap(SC_A);
    tap(SC_1);
    tap(SC_SLASH);
    keyboard_process_scancode(RELEASE(SC_LSHIFT));
    tap(SC_A);
    keyboard_process_scancode(SC_RSHIFT);
    tap(SC_B);
    keyboard_process_scancode(RELEASE(SC_RSHIFT));

    CHECK_EQ_INT(keyboard_try_read(), 'A');
    CHECK_EQ_INT(keyboard_try_read(), '!');
    CHECK_EQ_INT(keyboard_try_read(), '?');
    CHECK_EQ_INT(keyboard_try_read(), 'a');
    CHECK_EQ_INT(keyboard_try_read(), 'B');
}

TEST(keyboard_caps_lock_affects_letters_only) {
    keyboard_reset();
    tap(SC_CAPS);
    tap(SC_A);                              /* caps: A */
    tap(SC_1);                              /* digits unaffected: 1 */
    keyboard_process_scancode(SC_LSHIFT);
    tap(SC_A);                              /* caps + shift: a */
    keyboard_process_scancode(RELEASE(SC_LSHIFT));
    tap(SC_CAPS);
    tap(SC_A);                              /* caps off again: a */

    CHECK_EQ_INT(keyboard_try_read(), 'A');
    CHECK_EQ_INT(keyboard_try_read(), '1');
    CHECK_EQ_INT(keyboard_try_read(), 'a');
    CHECK_EQ_INT(keyboard_try_read(), 'a');
}

TEST(keyboard_held_caps_lock_toggles_once) {
    keyboard_reset();
    keyboard_process_scancode(SC_CAPS);     /* held: the keyboard repeats the press code */
    keyboard_process_scancode(SC_CAPS);
    keyboard_process_scancode(SC_CAPS);
    keyboard_process_scancode(RELEASE(SC_CAPS));
    tap(SC_A);
    CHECK_EQ_INT(keyboard_try_read(), 'A');
}

TEST(keyboard_ctrl_letters_give_control_codes) {
    keyboard_reset();
    keyboard_process_scancode(SC_CTRL);
    tap(SC_C);
    keyboard_process_scancode(RELEASE(SC_CTRL));
    keyboard_process_scancode(SC_EXTENDED);  /* right Ctrl = E0 1D */
    keyboard_process_scancode(SC_CTRL);
    tap(SC_A);
    keyboard_process_scancode(SC_EXTENDED);
    keyboard_process_scancode(RELEASE(SC_CTRL));
    tap(SC_C);

    CHECK_EQ_INT(keyboard_try_read(), 3);    /* Ctrl+C */
    CHECK_EQ_INT(keyboard_try_read(), 1);    /* Ctrl+A */
    CHECK_EQ_INT(keyboard_try_read(), 'c');  /* Ctrl released */
}

TEST(keyboard_extended_keys) {
    keyboard_reset();
    keyboard_process_scancode(SC_EXTENDED);
    keyboard_process_scancode(SC_ARROW_UP);
    keyboard_process_scancode(SC_EXTENDED);
    keyboard_process_scancode(RELEASE(SC_ARROW_UP));
    keyboard_process_scancode(SC_EXTENDED);
    keyboard_process_scancode(SC_ARROW_LEFT);
    tap(0x48);                               /* without E0 it's keypad 8 */

    CHECK_EQ_INT(keyboard_try_read(), KEY_UP);
    CHECK_EQ_INT(keyboard_try_read(), KEY_LEFT);
    CHECK_EQ_INT(keyboard_try_read(), '8');
    CHECK_EQ_INT(keyboard_try_read(), -1);
}

TEST(keyboard_full_buffer_drops_keys) {
    keyboard_reset();
    for (int i = 0; i < 200; i++) {
        tap(SC_A);
    }
    int count = 0;
    while (keyboard_try_read() >= 0) {
        count++;
    }
    CHECK_EQ_INT(count, 127);                /* 128 slots, one kept empty */
    tap(SC_B);                               /* still works afterwards */
    CHECK_EQ_INT(keyboard_try_read(), 'b');
}
