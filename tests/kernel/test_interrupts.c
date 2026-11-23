#include "test.h"
#include <cpu.h>
#include <interrupts.h>

#define VECTOR_BREAKPOINT     3
#define VECTOR_INVALID_OPCODE 6
#define VECTOR_GENERAL_PROTECTION 13

static volatile int calls;
static volatile uint64_t seen_vector;
static volatile uint64_t seen_error_code;

static void record(struct interrupt_frame* frame) {
    calls++;
    seen_vector = frame->vector;
    seen_error_code = frame->error_code;
}

/* ud2 is always 2 bytes long: resume right after it. */
static void record_and_skip_ud2(struct interrupt_frame* frame) {
    record(frame);
    frame->rip += 2;
}

/* Resume at a label placed right after the faulting instruction (see below). */
extern char test_gp_resume[];
static void record_and_resume_after_gp(struct interrupt_frame* frame) {
    record(frame);
    frame->rip = (uint64_t) test_gp_resume;
}

TEST(interrupts_are_enabled_for_tests) {
    CHECK(interrupts_enabled());
}

TEST(breakpoint_calls_handler_and_returns) {
    calls = 0;
    interrupt_set_handler(VECTOR_BREAKPOINT, record);
    __asm__ volatile ("int3");
    interrupt_set_handler(VECTOR_BREAKPOINT, NULL);

    CHECK_EQ_INT(calls, 1);
    CHECK_EQ_INT(seen_vector, VECTOR_BREAKPOINT);
    CHECK_EQ_INT(seen_error_code, 0);
}

/* Written by the asm below via RIP-relative addressing, so no register is needed. */
uint64_t test_saved_registers[14];

TEST(registers_survive_an_interrupt) {
    static const uint64_t expected[14] = {
        0x1100000000000011, 0x2200000000000022, 0x3300000000000033, 0x4400000000000044,
        0x5500000000000055, 0x6600000000000066, 0x7700000000000077, 0x8800000000000088,
        0x9900000000000099, 0xAA000000000000AA, 0xBB000000000000BB, 0xCC000000000000CC,
        0xDD000000000000DD, 0xEE000000000000EE,
    };
    static const char* const names[14] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8",
        "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    };

    interrupt_set_handler(VECTOR_BREAKPOINT, record);
    __asm__ volatile (
        "movabs $0x1100000000000011, %%rax\n\t"
        "movabs $0x2200000000000022, %%rbx\n\t"
        "movabs $0x3300000000000033, %%rcx\n\t"
        "movabs $0x4400000000000044, %%rdx\n\t"
        "movabs $0x5500000000000055, %%rsi\n\t"
        "movabs $0x6600000000000066, %%rdi\n\t"
        "movabs $0x7700000000000077, %%r8\n\t"
        "movabs $0x8800000000000088, %%r9\n\t"
        "movabs $0x9900000000000099, %%r10\n\t"
        "movabs $0xAA000000000000AA, %%r11\n\t"
        "movabs $0xBB000000000000BB, %%r12\n\t"
        "movabs $0xCC000000000000CC, %%r13\n\t"
        "movabs $0xDD000000000000DD, %%r14\n\t"
        "movabs $0xEE000000000000EE, %%r15\n\t"
        "int3\n\t"
        "mov %%rax, test_saved_registers+0(%%rip)\n\t"
        "mov %%rbx, test_saved_registers+8(%%rip)\n\t"
        "mov %%rcx, test_saved_registers+16(%%rip)\n\t"
        "mov %%rdx, test_saved_registers+24(%%rip)\n\t"
        "mov %%rsi, test_saved_registers+32(%%rip)\n\t"
        "mov %%rdi, test_saved_registers+40(%%rip)\n\t"
        "mov %%r8,  test_saved_registers+48(%%rip)\n\t"
        "mov %%r9,  test_saved_registers+56(%%rip)\n\t"
        "mov %%r10, test_saved_registers+64(%%rip)\n\t"
        "mov %%r11, test_saved_registers+72(%%rip)\n\t"
        "mov %%r12, test_saved_registers+80(%%rip)\n\t"
        "mov %%r13, test_saved_registers+88(%%rip)\n\t"
        "mov %%r14, test_saved_registers+96(%%rip)\n\t"
        "mov %%r15, test_saved_registers+104(%%rip)\n\t"
        :
        :
        : "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9",
          "r10", "r11", "r12", "r13", "r14", "r15", "memory");
    interrupt_set_handler(VECTOR_BREAKPOINT, NULL);

    for (int i = 0; i < 14; i++) {
        if (test_saved_registers[i] != expected[i]) {
            test_fail(__FILE__, __LINE__, "%s changed: 0x%016lx, expected 0x%016lx",
                      names[i], test_saved_registers[i], expected[i]);
        }
    }
}

TEST(invalid_opcode_handler_can_skip_the_instruction) {
    calls = 0;
    interrupt_set_handler(VECTOR_INVALID_OPCODE, record_and_skip_ud2);
    __asm__ volatile ("ud2");
    interrupt_set_handler(VECTOR_INVALID_OPCODE, NULL);

    CHECK_EQ_INT(calls, 1);
    CHECK_EQ_INT(seen_vector, VECTOR_INVALID_OPCODE);
    CHECK_EQ_INT(seen_error_code, 0);
}

TEST(general_protection_fault_reports_error_code) {
    /* Selector 0x1230 points far past the end of our 2-entry GDT: loading it
       into ds raises #GP, and the CPU pushes the selector as the error code. */
    calls = 0;
    interrupt_set_handler(VECTOR_GENERAL_PROTECTION, record_and_resume_after_gp);
    __asm__ volatile (
        "mov $0x1230, %%eax\n\t"
        "mov %%eax, %%ds\n\t"
        ".global test_gp_resume\n"
        "test_gp_resume:\n\t"
        : : : "rax", "memory");
    interrupt_set_handler(VECTOR_GENERAL_PROTECTION, NULL);

    CHECK_EQ_INT(calls, 1);
    CHECK_EQ_INT(seen_vector, VECTOR_GENERAL_PROTECTION);
    CHECK_EQ_INT(seen_error_code, 0x1230);
}
