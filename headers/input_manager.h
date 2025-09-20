#ifndef APOLLO_INPUT_MANAGER_H
#define APOLLO_INPUT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define SCANCODE_UP_ARROW 0x48
#define SCANCODE_DOWN_ARROW 0x50
#define SCANCODE_LEFT_ARROW 0x4B
#define SCANCODE_RIGHT_ARROW 0x4D
#define SCANCODE_ESCAPE 0x01
#define SCANCODE_ENTER 0x1C
#define SCANCODE_BACKSPACE 0x0E
#define SCANCODE_TAB 0x0F
#define SCANCODE_SPACE 0x39
#define SCANCODE_DELETE 0x53
#define SCANCODE_INSERT 0x52
#define SCANCODE_HOME 0x47
#define SCANCODE_END 0x4F
#define SCANCODE_PAGE_UP 0x49
#define SCANCODE_PAGE_DOWN 0x51

#define SCANCODE_F1 0xF1
#define SCANCODE_F2 0xF2
#define SCANCODE_F3 0xF3
#define SCANCODE_F4 0xF4
#define SCANCODE_F5 0xF5
#define SCANCODE_F6 0xF6
#define SCANCODE_F7 0xF7
#define SCANCODE_F8 0xF8
#define SCANCODE_F9 0xF9
#define SCANCODE_F10 0xFA
#define SCANCODE_F11 0xFB
#define SCANCODE_F12 0xFC

void input_manager_initialize(void);

bool input_manager_has_input(void);
uint8_t input_manager_read_scancode(void);

char input_manager_scancode_to_ascii(uint8_t scan_code);

bool input_manager_is_shift_pressed(void);
bool input_manager_is_ctrl_pressed(void);
bool input_manager_is_alt_pressed(void);

bool input_manager_is_caps_lock_on(void);
bool input_manager_is_num_lock_on(void);
bool input_manager_is_scroll_lock_on(void);

uint8_t input_manager_get_last_scancode(void);

#endif