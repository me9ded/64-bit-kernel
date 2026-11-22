#include <serial.h>
#include <cpu.h>
#include <stdbool.h>
#include <stddef.h>

#define COM1 0x3F8

/* UART register offsets from the base port. */
#define REG_DATA          0  /* data (or divisor low byte when DLAB=1) */
#define REG_INT_ENABLE    1  /* interrupt enable (or divisor high byte when DLAB=1) */
#define REG_FIFO_CONTROL  2
#define REG_LINE_CONTROL  3
#define REG_MODEM_CONTROL 4
#define REG_LINE_STATUS   5

#define LINE_STATUS_TX_EMPTY 0x20

static bool serial_ready = false;

int serial_init(void) {
    outb(COM1 + REG_INT_ENABLE, 0x00);    /* no UART interrupts (we have no IDT yet) */
    outb(COM1 + REG_LINE_CONTROL, 0x80);  /* DLAB=1: next two writes set the baud divisor */
    outb(COM1 + REG_DATA, 0x03);          /* divisor 3 -> 115200 / 3 = 38400 baud */
    outb(COM1 + REG_INT_ENABLE, 0x00);
    outb(COM1 + REG_LINE_CONTROL, 0x03);  /* DLAB=0, 8 data bits, no parity, 1 stop bit */
    outb(COM1 + REG_FIFO_CONTROL, 0xC7);  /* enable + clear FIFOs, 14-byte threshold */

    /* Loopback self-test: anything we send should come straight back. */
    outb(COM1 + REG_MODEM_CONTROL, 0x1E);
    outb(COM1 + REG_DATA, 0xAE);
    if (inb(COM1 + REG_DATA) != 0xAE) {
        return -1;
    }

    outb(COM1 + REG_MODEM_CONTROL, 0x0F); /* normal operation: DTR, RTS, OUT1, OUT2 */
    serial_ready = true;
    return 0;
}

static void serial_put_raw(char character) {
    while ((inb(COM1 + REG_LINE_STATUS) & LINE_STATUS_TX_EMPTY) == 0) {
        /* wait until the transmit buffer has room */
    }
    outb(COM1 + REG_DATA, (uint8_t) character);
}

void serial_write_char(char character) {
    if (!serial_ready) {
        return;
    }
    if (character == '\n') {
        serial_put_raw('\r');
    }
    serial_put_raw(character);
}

void serial_write_str(const char* string) {
    for (size_t i = 0; string[i] != '\0'; i++) {
        serial_write_char(string[i]);
    }
}
