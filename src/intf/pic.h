#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * 8259 Programmable Interrupt Controller (a master and a slave chip).
 * By default its IRQs collide with CPU exception vectors 8-15, so pic_init()
 * remaps IRQ 0-7 to vectors 32-39 and IRQ 8-15 to vectors 40-47.
 */

/* Remap and mask every IRQ (except IRQ 2, which links the slave to the master). */
void pic_init(void);

void pic_mask(uint8_t irq);
void pic_unmask(uint8_t irq);

/* Tell the PIC the IRQ has been handled, so it can deliver the next one. */
void pic_send_eoi(uint8_t irq);

/*
 * IRQ 7 and 15 can fire "spuriously" (electrical noise). Returns true if that
 * happened: the IRQ must then be ignored and must not get a normal EOI.
 */
bool pic_is_spurious(uint8_t irq);
