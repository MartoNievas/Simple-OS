#include "vga.h"
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VEGA_DEFAULT_COLOR 0x07

static volatile uint16_t *const vga_buffer =
    (volatile uint16_t *)(KERNEL_VBASE + 0xb8000);

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t color = VEGA_DEFAULT_COLOR;

static uint16_t vga_cell(char c, uint8_t attr) {
  return (uint16_t)(unsigned char)c | ((uint16_t)attr << 8);
}

static void vga_clear_row(int row) {
  for (int col = 0; col < VGA_WIDTH; col++) {
    vga_buffer[row * VGA_WIDTH + col] = vga_cell(' ', color);
  }
}

static void vga_scroll(void) {
  for (int row = 1; row < VGA_HEIGHT; row++) {
    for (int col = 0; col < VGA_WIDTH; col++) {
      vga_buffer[(row - 1) * VGA_WIDTH + col] =
          vga_buffer[row * VGA_WIDTH + col];
    }
  }

  vga_clear_row(VGA_HEIGHT - 1);
  cursor_y = VGA_HEIGHT - 1;
}

void vga_init(void) {
  for (int row = 0; row < VGA_HEIGHT; row++) {
    vga_clear_row(row);
  }
  cursor_x = 0;
  cursor_y = 0;
}

void vga_write_char(char c) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
  } else {
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = vga_cell(c, color);
    cursor_x++;
    if (cursor_x >= VGA_WIDTH) {
      cursor_x = 0;
      cursor_y++;
    }
  }

  if (cursor_y >= VGA_HEIGHT) {
    vga_scroll();
  }
}
