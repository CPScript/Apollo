#ifndef APOLLO_COMMAND_PROCESSOR_H
#define APOLLO_COMMAND_PROCESSOR_H

#include <stdint.h>
#include <stdbool.h>

void command_processor_initialize(void);

void command_processor_handle_input(uint8_t scan_code);

bool command_processor_is_initialized(void);
uint32_t command_processor_get_commands_executed(void);
const char* command_processor_get_current_user(void);

// Available commands:
// File System: ls, dir, cd, pwd, mkdir, rmdir, rm, cp, mv, cat, touch, find, tree, grep
// System Info: sysinfo, meminfo, df, ps, whoami, date, uptime
// Utilities: calc, echo, history, clear, edit, palette
// Control: reboot, shutdown, help

#endif