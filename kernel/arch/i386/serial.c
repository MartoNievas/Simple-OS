#include <stdint.h>

#include "io.h"
#include "serial.h"

#define COM1 0x3F8

static int transmit_empty(void) { return inb(COM1 + 5) & 0x20; }

int serial_init(void) {
  outb(COM1 + 1, 0x00); // disable interrupts
  outb(COM1 + 3, 0x80); // enable DLAB
  outb(COM1 + 0, 0x03); // divisor low byte: 3 -> 38400 baud
  outb(COM1 + 1, 0x00); // divisor high byte
  outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(COM1 + 2, 0xC7); // enable FIFO, clear it, 14-byte trigger
  outb(COM1 + 4, 0x0B); // RTS/DSR set
  outb(COM1 + 4, 0x1E); // loopback mode, for the self-test below

  outb(COM1 + 0, 0xAE);
  if (inb(COM1 + 0) != 0xAE) {
    return -1; // no working UART at this port
  }

  outb(COM1 + 4, 0x0F); // normal operation: loopback off, OUT1/OUT2 on
  return 0;
}

static void serial_put_raw(char c) {
  while (!transmit_empty()) {
  }
  outb(COM1, (uint8_t)c);
}

void serial_write_char(char c) {
  if (c == '\n') {
    serial_put_raw('\r'); // most serial terminals expect CRLF, not bare LF
  }
  serial_put_raw(c);
}
