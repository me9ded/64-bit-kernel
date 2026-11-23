#include <bootinfo.h>
#include <cpu.h>
#include <interrupts.h>
#include <keyboard.h>
#include <kprintf.h>
#include <print.h>
#include <serial.h>
#include <shell.h>
#include <timer.h>

/* multiboot_info: address of GRUB's boot information (see boot/main.asm) */
void kernel_main(uint64_t multiboot_info);
void kernel_main(uint64_t multiboot_info) {
    int serial_status = serial_init();

    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_clear();
    print_enable_cursor();
    kprintf("Welcome to our 64-bit kernel\n");
    kprintf("serial: %s\n", serial_status == 0 ? "COM1 ready" : "not found");

    if (bootinfo_init(multiboot_info) == 0) {
        kprintf("memory: %lu MiB usable\n", bootinfo_usable_bytes(boot_info()) >> 20);
    } else {
        kprintf("memory: boot information not found (at 0x%lx)\n", multiboot_info);
    }

    interrupts_init();
    kprintf("interrupts: IDT loaded, PIC remapped\n");

    timer_init(100);
    sti();
    kprintf("timer: %u Hz\n", timer_hz());

    sleep_ms(1000);
    kprintf("timer: 1 second passed (%lu ticks)\n", timer_ticks());

    keyboard_init();
    kprintf("keyboard: ready\n\n");

    kprintf("Type 'help' to see the available commands.\n");
    shell_run();
}
