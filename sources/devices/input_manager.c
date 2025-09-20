#include "input_manager.h"
#include <stdint.h>
#include <stdbool.h>

#define KB_DATA_PORT 0x60
#define KB_STATUS_PORT 0x64
#define KB_COMMAND_PORT 0x64

#define KB_STATUS_OUTPUT_FULL 0x01
#define KB_STATUS_INPUT_FULL 0x02

#define EXTENDED_SCANCODE_PREFIX 0xE0

static struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool caps_lock;
    bool num_lock;
    bool scroll_lock;
    bool extended_mode;
    uint8_t last_scancode;
} keyboard_state = {0};

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

static const char scancode_to_ascii[128] = {
    0,    27,   '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',  0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

static const char scancode_to_ascii_shifted[128] = {
    0,    27,   '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',  0,    ' ',  0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    '7',  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
    '2',  '3',  '0',  '.',  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};

static void update_keyboard_leds(void) {
    uint8_t led_state = 0;
    if (keyboard_state.scroll_lock) led_state |= 0x01;
    if (keyboard_state.num_lock) led_state |= 0x02;
    if (keyboard_state.caps_lock) led_state |= 0x04;
    
    while (inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL);
    outb(KB_DATA_PORT, 0xED);
    
    while (!(inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL));
    inb(KB_DATA_PORT);
    
    while (inb(KB_STATUS_PORT) & KB_STATUS_INPUT_FULL);
    outb(KB_DATA_PORT, led_state);
    
    while (!(inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL));
    inb(KB_DATA_PORT);
}

void input_manager_initialize(void) {
    keyboard_state.shift_pressed = false;
    keyboard_state.ctrl_pressed = false;
    keyboard_state.alt_pressed = false;
    keyboard_state.caps_lock = false;
    keyboard_state.num_lock = true;  // NumLock on by default
    keyboard_state.scroll_lock = false;
    keyboard_state.extended_mode = false;
    keyboard_state.last_scancode = 0;
    
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) {
        inb(KB_DATA_PORT);
    }
    
    update_keyboard_leds();
}

bool input_manager_has_input(void) {
    return (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT_FULL) != 0;
}

uint8_t input_manager_read_scancode(void) {
    if (!input_manager_has_input()) {
        return 0; // No data available
    }
    
    uint8_t scancode = inb(KB_DATA_PORT);
    keyboard_state.last_scancode = scancode;
    
    if (scancode == EXTENDED_SCANCODE_PREFIX) {
        keyboard_state.extended_mode = true;
        return 0;
    }
    
    return scancode;
}

char input_manager_scancode_to_ascii(uint8_t scan_code) {
    if (scan_code == 0) {
        return 0;
    }
    
    if (keyboard_state.extended_mode) {
        keyboard_state.extended_mode = false;
        
        if (scan_code & 0x80) {
            uint8_t key_code = scan_code & 0x7F;
            switch (key_code) {
                case 0x1D: // Right Ctrl
                    keyboard_state.ctrl_pressed = false;
                    break;
                case 0x38: // Right Alt
                    keyboard_state.alt_pressed = false;
                    break;
            }
            return 0;
        }
        
        switch (scan_code) {
            case 0x1D: // Right Ctrl
                keyboard_state.ctrl_pressed = true;
                return 0;
            case 0x38: // Right Alt
                keyboard_state.alt_pressed = true;
                return 0;
            case 0x48: // Up Arrow
                return SCANCODE_UP_ARROW;
            case 0x50: // Down Arrow
                return SCANCODE_DOWN_ARROW;
            case 0x4B: // Left Arrow
                return SCANCODE_LEFT_ARROW;
            case 0x4D: // Right Arrow
                return SCANCODE_RIGHT_ARROW;
            case 0x49: // Page Up
                return 0x49;
            case 0x51: // Page Down
                return 0x51;
            case 0x47: // Home
                return 0x47;
            case 0x4F: // End
                return 0x4F;
            case 0x52: // Insert
                return 0x52;
            case 0x53: // Delete
                return 0x53;
            default:
                return 0;
        }
    }
    
    if (scan_code & 0x80) {
        uint8_t key_code = scan_code & 0x7F;
        
        switch (key_code) {
            case 0x2A: // Left Shift
            case 0x36: // Right Shift
                keyboard_state.shift_pressed = false;
                break;
            case 0x1D: // Left Ctrl
                keyboard_state.ctrl_pressed = false;
                break;
            case 0x38: // Left Alt
                keyboard_state.alt_pressed = false;
                break;
        }
        return 0;
    }
    
    switch (scan_code) {
        case 0x2A: // Left Shift
        case 0x36: // Right Shift
            keyboard_state.shift_pressed = true;
            return 0;
        case 0x1D: // Left Ctrl
            keyboard_state.ctrl_pressed = true;
            return 0;
        case 0x38: // Left Alt
            keyboard_state.alt_pressed = true;
            return 0;
        case 0x3A: // Caps Lock
            keyboard_state.caps_lock = !keyboard_state.caps_lock;
            update_keyboard_leds();
            return 0;
        case 0x45: // Num Lock
            keyboard_state.num_lock = !keyboard_state.num_lock;
            update_keyboard_leds();
            return 0;
        case 0x46: // Scroll Lock
            keyboard_state.scroll_lock = !keyboard_state.scroll_lock;
            update_keyboard_leds();
            return 0;
        
        case 0x3B: return 0xF1; // F1
        case 0x3C: return 0xF2; // F2
        case 0x3D: return 0xF3; // F3
        case 0x3E: return 0xF4; // F4
        case 0x3F: return 0xF5; // F5
        case 0x40: return 0xF6; // F6
        case 0x41: return 0xF7; // F7
        case 0x42: return 0xF8; // F8
        case 0x43: return 0xF9; // F9
        case 0x44: return 0xFA; // F10
        case 0x57: return 0xFB; // F11
        case 0x58: return 0xFC; // F12
        
        default:
            if (scan_code < 128) {
                char base_char;
                
                if (keyboard_state.shift_pressed) {
                    base_char = scancode_to_ascii_shifted[scan_code];
                } else {
                    base_char = scancode_to_ascii[scan_code];
                }
                
                if (keyboard_state.caps_lock && base_char >= 'a' && base_char <= 'z') {
                    base_char = base_char - 'a' + 'A';
                } else if (keyboard_state.caps_lock && base_char >= 'A' && base_char <= 'Z') {
                    base_char = base_char - 'A' + 'a';
                }
                
                return base_char;
            }
            return 0;
    }
}

bool input_manager_is_shift_pressed(void) {
    return keyboard_state.shift_pressed;
}

bool input_manager_is_ctrl_pressed(void) {
    return keyboard_state.ctrl_pressed;
}

bool input_manager_is_alt_pressed(void) {
    return keyboard_state.alt_pressed;
}

bool input_manager_is_caps_lock_on(void) {
    return keyboard_state.caps_lock;
}

bool input_manager_is_num_lock_on(void) {
    return keyboard_state.num_lock;
}

bool input_manager_is_scroll_lock_on(void) {
    return keyboard_state.scroll_lock;
}

uint8_t input_manager_get_last_scancode(void) {
    return keyboard_state.last_scancode;
}