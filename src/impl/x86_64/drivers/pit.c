#include <timer.h>
#include <cpu.h>
#include <interrupts.h>
#include <kprintf.h>

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_BASE_HZ  1193182  /* the PIT's input clock */

/* channel 0, write low byte then high byte, mode 3 (square wave), binary */
#define PIT_MODE_SQUARE_WAVE 0x36

static volatile uint64_t ticks;
static uint32_t frequency;

static void timer_tick(struct interrupt_frame* frame) {
    (void) frame;
    ticks++;
}

void timer_init(uint32_t hz) {
    /* the divisor is a 16-bit value: hz below 19 would need more than 65535 */
    if (hz < 19 || hz > PIT_BASE_HZ) {
        panic("timer_init: %u Hz is out of range (19-%u)", hz, PIT_BASE_HZ);
    }

    uint32_t divisor = PIT_BASE_HZ / hz;
    frequency = hz;
    ticks = 0;

    outb(PIT_COMMAND, PIT_MODE_SQUARE_WAVE);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    irq_set_handler(0, timer_tick);
}

uint32_t timer_hz(void) {
    return frequency;
}

uint64_t timer_ticks(void) {
    return ticks;
}

uint64_t timer_uptime_ms(void) {
    if (frequency == 0) {
        return 0;
    }
    return ticks * 1000 / frequency;
}

void sleep_ms(uint64_t ms) {
    if (frequency == 0) {
        panic("sleep_ms() called before timer_init()");
    }
    if (!interrupts_enabled()) {
        panic("sleep_ms() called with interrupts disabled: it would never wake up");
    }

    uint64_t wait = (ms * frequency + 999) / 1000; /* round up to whole ticks */
    uint64_t target = ticks + wait;
    while (ticks < target) {
        hlt();
    }
}
