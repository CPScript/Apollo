#include "terminal.h"
#include <stdint.h>
#include <stddef.h>

#define VGA_MEMORY_BASE 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_CONTROL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5
#define VGA_PALETTE_MASK 0x3C6
#define VGA_PALETTE_READ_INDEX 0x3C7
#define VGA_PALETTE_WRITE_INDEX 0x3C8
#define VGA_PALETTE_DATA 0x3C9

typedef struct {
    uint8_t character;
    uint8_t attributes;
} vga_cell_t;

static struct {
    vga_cell_t *framebuffer;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint8_t current_fg_color;
    uint8_t current_bg_color;
    uint8_t combined_color;
} terminal_state;

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static void configure_vga_palette(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    while (inb(0x3DA) & 0x08);
    while (!(inb(0x3DA) & 0x08));
    
    outb(VGA_PALETTE_WRITE_INDEX, index);
    outb(VGA_PALETTE_DATA, red >> 2);   // VGA uses 6-bit color values
    outb(VGA_PALETTE_DATA, green >> 2);
    outb(VGA_PALETTE_DATA, blue >> 2);
}

static void initialize_default_palette(void) {
    configure_vga_palette(0, 0, 0, 0);         // Black
    configure_vga_palette(1, 0, 0, 170);       // Blue
    configure_vga_palette(2, 0, 170, 0);       // Green
    configure_vga_palette(3, 0, 170, 170);     // Cyan
    configure_vga_palette(4, 170, 0, 0);       // Red
    configure_vga_palette(5, 170, 0, 170);     // Magenta
    configure_vga_palette(6, 170, 85, 0);      // Brown
    configure_vga_palette(7, 170, 170, 170);   // Light Gray
    configure_vga_palette(8, 85, 85, 85);      // Dark Gray
    configure_vga_palette(9, 85, 85, 255);     // Light Blue
    configure_vga_palette(10, 85, 255, 85);    // Light Green
    configure_vga_palette(11, 85, 255, 255);   // Light Cyan
    configure_vga_palette(12, 255, 85, 85);    // Light Red
    configure_vga_palette(13, 255, 85, 255);   // Pink
    configure_vga_palette(14, 255, 255, 85);   // Yellow
    configure_vga_palette(15, 255, 255, 255);  // White
}

static void update_hardware_cursor(void) {
    uint16_t position = terminal_state.cursor_y * VGA_WIDTH + terminal_state.cursor_x;
    
    outb(VGA_CONTROL_REGISTER, 0x0F);
    outb(VGA_DATA_REGISTER, (uint8_t)(position & 0xFF));
    outb(VGA_CONTROL_REGISTER, 0x0E);
    outb(VGA_DATA_REGISTER, (uint8_t)((position >> 8) & 0xFF));
}

static void scroll_screen_up(void) {
    for (uint32_t row = 1; row < VGA_HEIGHT; row++) {
        for (uint32_t col = 0; col < VGA_WIDTH; col++) {
            uint32_t src_index = row * VGA_WIDTH + col;
            uint32_t dest_index = (row - 1) * VGA_WIDTH + col;
            terminal_state.framebuffer[dest_index] = terminal_state.framebuffer[src_index];
        }
    }
    
    for (uint32_t col = 0; col < VGA_WIDTH; col++) {
        uint32_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + col;
        terminal_state.framebuffer[index].character = ' ';
        terminal_state.framebuffer[index].attributes = terminal_state.combined_color;
    }
}

void terminal_initialize(void) {
    terminal_state.framebuffer = (vga_cell_t *)VGA_MEMORY_BASE;
    terminal_state.cursor_x = 0;
    terminal_state.cursor_y = 0;
    terminal_state.current_fg_color = 15; // White
    terminal_state.current_bg_color = 0;  // Black
    terminal_state.combined_color = (terminal_state.current_bg_color << 4) | terminal_state.current_fg_color;
    
    initialize_default_palette();
    terminal_clear();
}

void terminal_clear(void) {
    for (uint32_t row = 0; row < VGA_HEIGHT; row++) {
        for (uint32_t col = 0; col < VGA_WIDTH; col++) {
            uint32_t index = row * VGA_WIDTH + col;
            terminal_state.framebuffer[index].character = ' ';
            terminal_state.framebuffer[index].attributes = terminal_state.combined_color;
        }
    }
    
    terminal_state.cursor_x = 0;
    terminal_state.cursor_y = 0;
    update_hardware_cursor();
}

void terminal_write_char(char c) {
    if (c == '\n') {
        terminal_state.cursor_x = 0;
        terminal_state.cursor_y++;
        
        if (terminal_state.cursor_y >= VGA_HEIGHT) {
            scroll_screen_up();
            terminal_state.cursor_y = VGA_HEIGHT - 1;
        }
        
        update_hardware_cursor();
        return;
    }
    
    if (terminal_state.cursor_x >= VGA_WIDTH) {
        terminal_state.cursor_x = 0;
        terminal_state.cursor_y++;
        
        if (terminal_state.cursor_y >= VGA_HEIGHT) {
            scroll_screen_up();
            terminal_state.cursor_y = VGA_HEIGHT - 1;
        }
    }
    
    uint32_t index = terminal_state.cursor_y * VGA_WIDTH + terminal_state.cursor_x;
    terminal_state.framebuffer[index].character = c;
    terminal_state.framebuffer[index].attributes = terminal_state.combined_color;
    
    terminal_state.cursor_x++;
    update_hardware_cursor();
}

void terminal_write_string(const char* str) {
    while (*str) {
        terminal_write_char(*str);
        str++;
    }
}

void terminal_set_color(uint8_t foreground, uint8_t background) {
    terminal_state.current_fg_color = foreground & 0x0F;
    terminal_state.current_bg_color = background & 0x0F;
    terminal_state.combined_color = (terminal_state.current_bg_color << 4) | terminal_state.current_fg_color;
}

void terminal_set_custom_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    configure_vga_palette(index, red, green, blue);
}

void terminal_enable_cursor(void) {
    outb(VGA_CONTROL_REGISTER, 0x0A);
    uint8_t cursor_start = inb(VGA_DATA_REGISTER) & 0xC0;
    outb(VGA_CONTROL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, cursor_start | 13);
    
    outb(VGA_CONTROL_REGISTER, 0x0B);
    outb(VGA_DATA_REGISTER, 14);
}

void terminal_disable_cursor(void) {
    outb(VGA_CONTROL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, 0x20);
}

void terminal_backspace(void) {
    if (terminal_state.cursor_x > 0) {
        terminal_state.cursor_x--;
    } else if (terminal_state.cursor_y > 0) {
        terminal_state.cursor_y--;
        terminal_state.cursor_x = VGA_WIDTH - 1;
        
        while (terminal_state.cursor_x > 0) {
            uint32_t index = terminal_state.cursor_y * VGA_WIDTH + terminal_state.cursor_x;
            if (terminal_state.framebuffer[index].character != ' ') {
                terminal_state.cursor_x++;
                break;
            }
            terminal_state.cursor_x--;
        }
    } else {
        return; 
    }
    
    uint32_t index = terminal_state.cursor_y * VGA_WIDTH + terminal_state.cursor_x;
    terminal_state.framebuffer[index].character = ' ';
    terminal_state.framebuffer[index].attributes = terminal_state.combined_color;
    
    update_hardware_cursor();
}

void terminal_write_uint(uint32_t value) {
    if (value == 0) {
        terminal_write_char('0');
        return;
    }
    
    char buffer[11]; // Max uint32_t is 10 digits + null terminator
    int pos = 0;
    
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    for (int i = pos - 1; i >= 0; i--) {
        terminal_write_char(buffer[i]);
    }
}

void terminal_write_int(int32_t value) {
    if (value < 0) {
        terminal_write_char('-');
        terminal_write_uint((uint32_t)(-value));
    } else {
        terminal_write_uint((uint32_t)value);
    }
}

void terminal_write_hex(uintptr_t value) {
    const char hex_digits[] = "0123456789ABCDEF";
    char buffer[17]; // 64-bit hex + null terminator
    int pos = 0;
    
    if (value == 0) {
        terminal_write_string("0x0");
        return;
    }
    
    while (value > 0) {
        buffer[pos++] = hex_digits[value % 16];
        value /= 16;
    }
    
    terminal_write_string("0x");
    for (int i = pos - 1; i >= 0; i--) {
        terminal_write_char(buffer[i]);
    }
}