#include <cpu.h>
#include <interrupts.h>
#include <keyboard.h>
#include <kprintf.h>
#include <print.h>
#include <readline.h>
#include <serial.h>
#include <timer.h>

void kernel_main(void) {
    int serial_status = serial_init();

    print_clear();
    print_enable_cursor();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    kprintf("Welcome to our 64-bit kernel\n");
    kprintf("serial: %s\n", serial_status == 0 ? "COM1 ready" : "not found");

    interrupts_init();
    kprintf("interrupts: IDT loaded, PIC remapped\n");

    timer_init(100);
    sti();
    kprintf("timer: %u Hz\n", timer_hz());

    sleep_ms(1000);
    kprintf("timer: 1 second passed (%lu ticks)\n", timer_ticks());

    keyboard_init();
    kprintf("keyboard: ready\n\n");

    /* Echo loop: this becomes the shell in step 4. */
    char line[128];
    for (;;) {
        kprintf("> ");
        if (readline(line, sizeof line) < 0) {
            kprintf("(cancelled)\n");
        } else {
            kprintf("you typed: %s\n", line);
        }
    }
}
