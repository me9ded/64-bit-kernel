#include <interrupts.h>
#include <cpu.h>
#include <kprintf.h>
#include <pic.h>
#include <stddef.h>

#define KERNEL_CODE_SELECTOR 0x08   /* gdt64.code_segment in boot/main.asm */
#define GATE_INTERRUPT_64    0x8E   /* present, ring 0, 64-bit interrupt gate (clears IF) */
#define STUB_COUNT           48     /* vectors with a stub in isr.asm */
#define VECTOR_PAGE_FAULT    14

_Static_assert(sizeof(struct interrupt_frame) == 22 * 8,
               "struct interrupt_frame must match the stack layout built by isr.asm");

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;                /* 0 = keep using the current stack */
    uint8_t type_attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static interrupt_handler_t handlers[256];

extern const uint64_t isr_stub_table[STUB_COUNT];

static const struct {
    const char* name;
    const char* mnemonic;
} exceptions[32] = {
    { "Divide error", "#DE" },
    { "Debug", "#DB" },
    { "Non-maskable interrupt", "NMI" },
    { "Breakpoint", "#BP" },
    { "Overflow", "#OF" },
    { "Bound range exceeded", "#BR" },
    { "Invalid opcode", "#UD" },
    { "Device not available", "#NM" },
    { "Double fault", "#DF" },
    { "Coprocessor segment overrun", "-" },
    { "Invalid TSS", "#TS" },
    { "Segment not present", "#NP" },
    { "Stack-segment fault", "#SS" },
    { "General protection fault", "#GP" },
    { "Page fault", "#PF" },
    { "Reserved", "-" },
    { "x87 floating-point error", "#MF" },
    { "Alignment check", "#AC" },
    { "Machine check", "#MC" },
    { "SIMD floating-point error", "#XM" },
    { "Virtualization exception", "#VE" },
    { "Control protection exception", "#CP" },
    { "Reserved", "-" },
    { "Reserved", "-" },
    { "Reserved", "-" },
    { "Reserved", "-" },
    { "Reserved", "-" },
    { "Reserved", "-" },
    { "Hypervisor injection exception", "#HV" },
    { "VMM communication exception", "#VC" },
    { "Security exception", "#SX" },
    { "Reserved", "-" },
};

static void set_gate(uint8_t vector, uint64_t stub_address) {
    idt[vector] = (struct idt_entry) {
        .offset_low = stub_address & 0xFFFF,
        .selector = KERNEL_CODE_SELECTOR,
        .ist = 0,
        .type_attributes = GATE_INTERRUPT_64,
        .offset_mid = (stub_address >> 16) & 0xFFFF,
        .offset_high = stub_address >> 32,
        .reserved = 0,
    };
}

void interrupts_init(void) {
    for (uint8_t vector = 0; vector < STUB_COUNT; vector++) {
        set_gate(vector, isr_stub_table[vector]);
    }

    struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t) idt,
    };
    __asm__ volatile ("lidt %0" : : "m"(pointer));

    pic_init();
}

void interrupt_set_handler(uint8_t vector, interrupt_handler_t handler) {
    handlers[vector] = handler;
}

void irq_set_handler(uint8_t irq, interrupt_handler_t handler) {
    handlers[IRQ_BASE_VECTOR + irq] = handler;
    if (handler != NULL) {
        pic_unmask(irq);
    } else {
        pic_mask(irq);
    }
}

__attribute__((noreturn)) static void unhandled_exception(const struct interrupt_frame* frame) {
    char page_fault_address[48] = "";
    if (frame->vector == VECTOR_PAGE_FAULT) {
        ksnprintf(page_fault_address, sizeof page_fault_address,
                  "  cr2=0x%016lx (address accessed)\n", read_cr2());
    }

    panic("%s (%s, vector %lu), error code 0x%lx\n"
          "  rip=0x%016lx rsp=0x%016lx rflags=0x%lx\n"
          "%s"
          "  rax=0x%016lx rbx=0x%016lx rcx=0x%016lx\n"
          "  rdx=0x%016lx rsi=0x%016lx rdi=0x%016lx\n"
          "  rbp=0x%016lx r8 =0x%016lx r9 =0x%016lx\n"
          "  r10=0x%016lx r11=0x%016lx r12=0x%016lx\n"
          "  r13=0x%016lx r14=0x%016lx r15=0x%016lx",
          exceptions[frame->vector].name, exceptions[frame->vector].mnemonic,
          frame->vector, frame->error_code,
          frame->rip, frame->rsp, frame->rflags,
          page_fault_address,
          frame->rax, frame->rbx, frame->rcx,
          frame->rdx, frame->rsi, frame->rdi,
          frame->rbp, frame->r8, frame->r9,
          frame->r10, frame->r11, frame->r12,
          frame->r13, frame->r14, frame->r15);
}

/* Called from isr.asm for every interrupt, with interrupts disabled. */
void interrupt_dispatch(struct interrupt_frame* frame);
void interrupt_dispatch(struct interrupt_frame* frame) {
    uint64_t vector = frame->vector;
    interrupt_handler_t handler = handlers[vector];

    if (vector >= IRQ_BASE_VECTOR) {
        uint8_t irq = (uint8_t) (vector - IRQ_BASE_VECTOR);
        if (pic_is_spurious(irq)) {
            return;
        }
        if (handler != NULL) {
            handler(frame);
        }
        pic_send_eoi(irq);
        return;
    }

    if (handler != NULL) {
        handler(frame);
        return;
    }
    unhandled_exception(frame);
}
