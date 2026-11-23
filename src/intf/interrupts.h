#pragma once

#include <stdint.h>

/*
 * Interrupts and CPU exceptions.
 *
 *   vectors  0-31  CPU exceptions (divide error, page fault, ...)
 *   vectors 32-47  hardware IRQs 0-15, after the PIC is remapped (see pic.h)
 *
 * An exception without a registered handler panics with the exception name,
 * error code and registers. An IRQ without a handler is acknowledged and ignored.
 */

#define IRQ_BASE_VECTOR 32

/*
 * Everything saved on the stack when an interrupt arrives. The layout must
 * match src/impl/x86_64/cpu/isr.asm: general-purpose registers (pushed by the
 * stub), vector + error code (pushed by the stub or the CPU), then the five
 * values the CPU pushes itself. A handler may change e.g. `rip`: execution
 * resumes from whatever the frame says.
 */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector;
    uint64_t error_code; /* 0 for vectors where the CPU doesn't push one */
    uint64_t rip, cs, rflags, rsp, ss;
};

typedef void (*interrupt_handler_t)(struct interrupt_frame* frame);

/* Build and load the IDT and remap the PIC. Does NOT enable interrupts (use sti()). */
void interrupts_init(void);

/* Handle a vector yourself. Pass NULL to restore the default behaviour. */
void interrupt_set_handler(uint8_t vector, interrupt_handler_t handler);

/*
 * Handle hardware IRQ 0-15. Also unmasks the IRQ at the PIC (or masks it when
 * handler is NULL). End-of-interrupt is sent for you after the handler returns.
 */
void irq_set_handler(uint8_t irq, interrupt_handler_t handler);
