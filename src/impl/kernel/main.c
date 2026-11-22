#include <print.h>
#include <serial.h>
#include <kprintf.h>

void kernel_main(void) {
    int serial_status = serial_init();

    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    kprintf("Welcome to our 64-bit kernel\n");
    kprintf("serial: %s\n", serial_status == 0 ? "COM1 ready" : "not found");
}
