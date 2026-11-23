#pragma once

#include <stdbool.h>
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

/* Write / read two bytes (a "word") on an x86 I/O port. */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
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

/* Disable / enable maskable interrupts (hardware IRQs). */
static inline void cli(void) {
    __asm__ volatile ("cli" ::: "memory");
}

static inline void sti(void) {
    __asm__ volatile ("sti" ::: "memory");
}

/* Sleep until the next interrupt arrives. */
static inline void hlt(void) {
    __asm__ volatile ("hlt" ::: "memory");
}

/* After a page fault, CR2 holds the address the CPU tried to access. */
static inline uint64_t read_cr2(void) {
    uint64_t value;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(value));
    return value;
}

static inline uint64_t read_rflags(void) {
    uint64_t flags;
    __asm__ volatile ("pushfq\n\tpop %0" : "=r"(flags) : : "memory");
    return flags;
}

/* True if the IF flag is set, i.e. hardware interrupts can arrive. */
static inline bool interrupts_enabled(void) {
    return (read_rflags() & (1 << 9)) != 0;
}
