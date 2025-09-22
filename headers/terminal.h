#ifndef APOLLO_TERMINAL_H
#define APOLLO_TERMINAL_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_PINK = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

void terminal_initialize(void);
void terminal_clear(void);

void terminal_write_char(char c);
void terminal_write_string(const char* str);

void terminal_set_color(uint8_t foreground, uint8_t background);
void terminal_set_custom_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);

void terminal_enable_cursor(void);
void terminal_disable_cursor(void);
void terminal_backspace(void);

void terminal_handle_scroll_input(uint8_t scan_code);
void terminal_show_scroll_status(void);

void terminal_write_uint(uint32_t value);
void terminal_write_int(int32_t value);
void terminal_write_hex(uintptr_t value);

#endif