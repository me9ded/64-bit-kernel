#include <cpu.h>
#include <interrupts.h>
#include <kprintf.h>
#include <print.h>
#include <serial.h>
#include <timer.h>

void kernel_main(void) {
    int serial_status = serial_init();

    print_clear();
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

    /* Nothing left to do: sleep until the next interrupt, forever. */
    for (;;) {
        hlt();
    }
}
