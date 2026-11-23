#include <pic.h>
#include <cpu.h>
#include <interrupts.h>

#define PIC1_COMMAND 0x20   /* master */
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0   /* slave */
#define PIC2_DATA    0xA1

#define ICW1_INIT     0x10  /* start initialization */
#define ICW1_ICW4     0x01  /* ICW4 will follow */
#define ICW4_8086     0x01  /* 8086 mode */
#define CMD_EOI       0x20  /* end of interrupt */
#define CMD_READ_ISR  0x0B  /* next read returns the in-service register */

#define CASCADE_IRQ 2

void pic_init(void) {
    /* ICW1: start the initialization sequence on both chips */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: first vector of each chip */
    outb(PIC1_DATA, IRQ_BASE_VECTOR);
    io_wait();
    outb(PIC2_DATA, IRQ_BASE_VECTOR + 8);
    io_wait();

    /* ICW3: the slave is wired to the master's IRQ 2 */
    outb(PIC1_DATA, 1 << CASCADE_IRQ);
    io_wait();
    outb(PIC2_DATA, CASCADE_IRQ);
    io_wait();

    /* ICW4: 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Mask everything; drivers unmask their IRQ via irq_set_handler(). */
    outb(PIC1_DATA, (uint8_t) ~(1 << CASCADE_IRQ));
    outb(PIC2_DATA, 0xFF);
}

static uint16_t data_port(uint8_t irq) {
    return irq < 8 ? PIC1_DATA : PIC2_DATA;
}

void pic_mask(uint8_t irq) {
    uint16_t port = data_port(irq);
    outb(port, inb(port) | (uint8_t) (1 << (irq % 8)));
}

void pic_unmask(uint8_t irq) {
    uint16_t port = data_port(irq);
    outb(port, inb(port) & (uint8_t) ~(1 << (irq % 8)));
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, CMD_EOI);
    }
    outb(PIC1_COMMAND, CMD_EOI);
}

static uint16_t read_in_service(void) {
    outb(PIC1_COMMAND, CMD_READ_ISR);
    outb(PIC2_COMMAND, CMD_READ_ISR);
    return (uint16_t) ((inb(PIC2_COMMAND) << 8) | inb(PIC1_COMMAND));
}

bool pic_is_spurious(uint8_t irq) {
    if (irq != 7 && irq != 15) {
        return false;
    }
    if (read_in_service() & (1 << irq)) {
        return false; /* a real IRQ 7/15 */
    }
    if (irq == 15) {
        /* the master did see a real IRQ 2 from the slave, so it needs its EOI */
        outb(PIC1_COMMAND, CMD_EOI);
    }
    return true;
}
