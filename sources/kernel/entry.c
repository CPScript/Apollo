#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "input_manager.h"
#include "command_processor.h"
#include "heap_allocator.h"
#include "time_keeper.h"

#define APOLLO_VERSION "1.0"
#define APOLLO_ARCH "x86_64"

static void apollo_display_startup_sequence(void) {
    terminal_initialize();
    
    terminal_set_custom_color(1, 255, 165, 0);   // Orange
    terminal_set_custom_color(2, 255, 215, 0);   // Gold
    terminal_set_custom_color(3, 255, 69, 0);    // Red-Orange
    terminal_set_custom_color(4, 255, 140, 0);   // Dark Orange
    terminal_set_custom_color(5, 255, 20, 147);  // Deep Pink
    terminal_set_custom_color(6, 138, 43, 226);  // Blue Violet
    terminal_set_custom_color(7, 255, 255, 255); // White
    
    terminal_set_color(14, 0);
    terminal_write_string("          \\   |   /\n");
    terminal_set_color(3, 0);
    terminal_write_string("       --  .----.  --\n");
    terminal_set_color(14, 0);
    terminal_write_string("          /      \\\n");
    terminal_write_string("         | APOLLO |\n");
    terminal_write_string("          \\      /\n");
    terminal_set_color(3, 0);
    terminal_write_string("       --  '----'  --\n");
    terminal_set_color(14, 0);
    terminal_write_string("          /   |   \\\n\n");
    terminal_set_color(3, 0);
    terminal_write_string("Welcome to\n");
    terminal_set_color(14, 0);
    terminal_write_string("Apollo Kernel\n");
    
    terminal_set_color(7, 0);
    
    terminal_write_string("System Palette: ");
    for (int palette_index = 0; palette_index < 16; palette_index++) {
        terminal_set_color(palette_index, palette_index);
        terminal_write_char(' ');
    }
    terminal_set_color(7, 0);
    terminal_write_string("\n\n");
    
    terminal_write_string("ApolloKernel v");
    terminal_write_string(APOLLO_VERSION);
    terminal_write_string(" - x86_64 Operating System\n");
    terminal_write_string("Architecture: ");
    terminal_write_string(APOLLO_ARCH);
    terminal_write_string("\n");
    terminal_write_string("Build: ");
    terminal_write_string(__DATE__);
    terminal_write_string(" ");
    terminal_write_string(__TIME__);
    terminal_write_string("\n");
    
    #ifdef __GNUC__
        terminal_write_string("Compiler: GCC ");
        terminal_write_string(__VERSION__);
    #endif
    
    terminal_write_string("\n\nMemory Manager: Initialized\n");
    terminal_write_string("VGA Driver: Ready\n");
    terminal_write_string("Keyboard Driver: Active\n");
    terminal_write_string("Real-Time Clock: Online\n");
    terminal_write_string("Shell System: Loaded\n\n");
    
    terminal_write_string("Type 'shell' to enter the Apollo command environment\n");
    terminal_write_string("Type 'help' for available commands\n");
    terminal_write_string("Press Ctrl+Alt+G in QEMU to capture keyboard input\n\n");
    terminal_write_string("apollo> ");
    
    terminal_enable_cursor();
}

static void apollo_initialize_subsystems(void) {
    heap_allocator_initialize();
    
    terminal_initialize();
    
    apollo_display_startup_sequence();
    
    terminal_write_string("Initializing PS/2 keyboard controller...\n");
    input_manager_initialize();
    terminal_write_string("Keyboard initialized successfully.\n\n");
    
    command_processor_initialize();
    
    terminal_write_string("apollo> ");
}

static void cpu_relax(void) {
    for (volatile int i = 0; i < 1000; i++) {
        __asm__ volatile("pause");
    }
}

void apollo_kernel_main(void) {
    apollo_initialize_subsystems();
    
    while (1) {
        if (input_manager_has_input()) {
            uint8_t scan_code = input_manager_read_scancode();
            command_processor_handle_input(scan_code);
        }
        
        cpu_relax();
    }
}