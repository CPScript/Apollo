#ifndef APOLLO_COMMAND_PROCESSOR_H
#define APOLLO_COMMAND_PROCESSOR_H

#include <stdint.h>

void command_processor_initialize(void);

void command_processor_handle_input(uint8_t scan_code);

#endif 