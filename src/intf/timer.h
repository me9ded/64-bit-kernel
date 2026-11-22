#pragma once

#include <stdint.h>

/*
 * System timer, driven by the PIT (programmable interval timer) on IRQ 0.
 * Requires interrupts_init() first, and sti() for ticks to arrive.
 */

/* Start ticking `hz` times per second (19-1193182; 100 is typical). */
void timer_init(uint32_t hz);

uint32_t timer_hz(void);
uint64_t timer_ticks(void);      /* ticks since timer_init() */
uint64_t timer_uptime_ms(void);  /* milliseconds since timer_init() */

/* Wait at least `ms` milliseconds, halting the CPU in between ticks. */
void sleep_ms(uint64_t ms);
