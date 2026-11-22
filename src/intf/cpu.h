#pragma once

#include <stdint.h>

/* Write one byte to an x86 I/O port. */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* Read one byte from an x86 I/O port. */
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Tiny delay for slow legacy devices: port 0x80 is an unused POST-code port. */
static inline void io_wait(void) {
    outb(0x80, 0);
}

/* Stop the CPU for good: disable interrupts, then halt in a loop. */
__attribute__((noreturn)) static inline void halt_forever(void) {
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
