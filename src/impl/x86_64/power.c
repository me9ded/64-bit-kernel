#include <power.h>
#include <cpu.h>
#include <kprintf.h>

#define PS2_STATUS             0x64
#define PS2_COMMAND            0x64
#define PS2_STATUS_INPUT_FULL  0x02
#define PS2_PULSE_RESET_LINE   0xFE

void reboot(void) {
    cli();

    /* Ask the keyboard controller to pulse the CPU's reset line. */
    for (int i = 0; i < 100000 && (inb(PS2_STATUS) & PS2_STATUS_INPUT_FULL); i++) {
    }
    outb(PS2_COMMAND, PS2_PULSE_RESET_LINE);
    for (int i = 0; i < 1000; i++) {
        io_wait();
    }

    /* Still running: load an empty IDT and raise an exception. With no way to
       handle it the CPU triple-faults, which also resets the machine. */
    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) empty_idt = { 0, 0 };
    __asm__ volatile ("lidt %0\n\tint3" : : "m"(empty_idt));
    halt_forever();
}

void shutdown(void) {
    /* Emulator-specific power-off ports (real hardware needs ACPI). */
    outw(0x604, 0x2000);   /* QEMU */
    outw(0xB004, 0x2000);  /* Bochs, older QEMU */
    outw(0x4004, 0x3400);  /* VirtualBox */

    kprintf("This machine can't be powered off automatically. It's now safe to turn it off.\n");
    halt_forever();
}
