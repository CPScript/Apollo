#include <stdint.h>
#include <stddef.h>
#include "terminal.h"
#include "input_manager.h"
#include "command_processor.h"
#include "heap_allocator.h"
#include "time_keeper.h"
#include "filesystem.h"
#include "process_manager.h"
#include "text_editor.h"

#define APOLLO_VERSION "1.1.2"
#define APOLLO_ARCH "x86_64"
#define APOLLO_BUILD_DATE __DATE__
#define APOLLO_BUILD_TIME __TIME__

static struct {
    uint32_t boot_time;
    uint32_t initialization_steps;
    bool all_systems_ready;
} system_state = {0};

static void apollo_display_startup_sequence(void) {
    terminal_initialize();
    
    terminal_set_custom_color(1, 255, 165, 0);   // Orange
    terminal_set_custom_color(2, 255, 215, 0);   // Gold
    terminal_set_custom_color(3, 255, 69, 0);    // Red-Orange
    terminal_set_custom_color(4, 255, 140, 0);   // Dark Orange
    terminal_set_custom_color(5, 255, 20, 147);  // Deep Pink
    terminal_set_custom_color(6, 138, 43, 226);  // Blue Violet
    terminal_set_custom_color(7, 255, 255, 255); // White
    
    terminal_clear();
    
    terminal_set_color(14, 0); // Yellow
    terminal_write_string("          \\   |   /\n");
    terminal_set_color(3, 0);  // Orange
    terminal_write_string("       --  .----.  --\n");
    terminal_set_color(14, 0); // Yellow
    terminal_write_string("          /      \\\n");
    terminal_write_string("         | APOLLO |\n");
    terminal_write_string("          \\      /\n");
    terminal_set_color(3, 0);  // Orange
    terminal_write_string("       --  '----'  --\n");
    terminal_set_color(14, 0); // Yellow
    terminal_write_string("          /   |   \\\n\n");
    terminal_set_color(3, 0);  // Orange
    terminal_write_string("Welcome to\n");
    terminal_set_color(14, 0); // Yellow
    terminal_write_string("Apollo Operating System\n");
    terminal_set_color(7, 0);
    
    terminal_write_string("\nApollo Kernel v");
    terminal_write_string(APOLLO_VERSION);
    terminal_write_string(" - x86_64 Operating System\n");
    terminal_write_string("Build: ");
    terminal_write_string(APOLLO_BUILD_DATE);
    terminal_write_string(" ");
    terminal_write_string(APOLLO_BUILD_TIME);
    terminal_write_string("\n");
    terminal_write_string("Architecture: ");
    terminal_write_string(APOLLO_ARCH);
    terminal_write_string("\n");
    
    #ifdef __GNUC__
        terminal_write_string("Compiler: GCC ");
        terminal_write_string(__VERSION__);
        terminal_write_string("\n");
    #endif
    
    terminal_write_string("\nSystem Palette: ");
    for (int i = 0; i < 16; i++) {
        terminal_set_color(i, i);
        terminal_write_char(' ');
    }
    terminal_set_color(7, 0);
    terminal_write_string("\n\n");
}

static void apollo_initialize_all_systems(void) {
    terminal_write_string("Initializing Apollo Operating System...\n");
    
    heap_allocator_initialize();
    
    time_keeper_initialize();
    system_state.boot_time = time_keeper_get_uptime_seconds();
    
    filesystem_initialize();
    
    process_manager_initialize();
    
    terminal_initialize(); // Re-initialize for consistency
    
    input_manager_initialize();
    
    text_editor_initialize();
    
    command_processor_initialize();
    
    uint32_t heap_total = heap_allocator_get_total_memory();
    fs_stats_t fs_stats;
    filesystem_get_stats(&fs_stats);
    process_stats_t proc_stats;
    process_get_stats(&proc_stats);
    
    bool systems_ok = (heap_total > 0) && 
                      (fs_stats.total_directories > 0) && 
                      (proc_stats.total_processes > 0);
    
    system_state.all_systems_ready = systems_ok;
    
    if (systems_ok) {
        terminal_set_color(10, 0); // Green
        
        apollo_display_startup_sequence();

        terminal_write_string("\nSystem Status:\n");
        terminal_write_string("  Memory Available:  ");
        terminal_write_uint(heap_allocator_get_free_memory() / 1024);
        terminal_write_string(" KB\n");
        terminal_write_string("  Files Available:   ");
        terminal_write_uint(fs_stats.total_files);
        terminal_write_string(" files in ");
        terminal_write_uint(fs_stats.total_directories);
        terminal_write_string(" directories\n");
        terminal_write_string("  Processes Running: ");
        terminal_write_uint(proc_stats.total_processes);
        terminal_write_string(" (");
        terminal_write_uint(proc_stats.running_processes);
        terminal_write_string(" active)\n");
        terminal_write_string("  Free Disk Space:   ");
        terminal_write_uint(filesystem_get_free_space() / 1024);
        terminal_write_string(" KB\n");

        terminal_write_string("\nQuick Start Commands:\n");
        terminal_set_color(11, 0); // Light cyan
        terminal_write_string("  help        ");
        terminal_set_color(7, 0);
        terminal_write_string("- Complete command reference\n");
        terminal_set_color(11, 0);
        terminal_write_string("  edit        ");
        terminal_set_color(7, 0);
        terminal_write_string("- Text editor with real file I/O\n");
        terminal_set_color(11, 0);
        terminal_write_string("  sysinfo     ");
        terminal_set_color(7, 0);
        terminal_write_string("- Complete system information\n");
        terminal_set_color(11, 0);
        terminal_write_string("  ls          ");
        terminal_set_color(7, 0);
        terminal_write_string("- List files and directories\n");
        terminal_write_string("\n");
        terminal_set_color(7, 0);
        
    } else {
        terminal_set_color(12, 0); // Red
        terminal_write_string("CRITICAL ERROR: System initialization failed!\n");
        terminal_set_color(7, 0);
    }
    
    terminal_write_string("apollo> ");
    terminal_enable_cursor();
}

static void cpu_relax(void) {
    for (volatile int i = 0; i < 500; i++) {
        __asm__ volatile("pause");
    }
}

static void update_system_activity(void) {
    extern void process_simulate_activity(void);
    process_simulate_activity();
    
    extern void process_scheduler_tick(void);
    process_scheduler_tick();
    
    uint32_t current_heap_usage = heap_allocator_get_used_memory();
    extern void process_update_memory_usage(uint32_t pid, uint32_t memory_bytes);
    process_update_memory_usage(5, current_heap_usage); // Shell process
}

void apollo_kernel_main(void) {
    
    terminal_initialize();

    apollo_initialize_all_systems();
    
    if (!system_state.all_systems_ready) {
        terminal_set_color(12, 0);
        terminal_write_string("FATAL: Cannot start system - initialization failed\n");
        terminal_set_color(7, 0);
        
        __asm__ __volatile__("cli; hlt");
        return;
    }
    
    uint32_t activity_counter = 0;
    
    while (1) {
        if (input_manager_has_input()) {
            uint8_t scan_code = input_manager_read_scancode();
            command_processor_handle_input(scan_code);
        }
        
        activity_counter++;
        if (activity_counter >= 10000) {
            update_system_activity();
            activity_counter = 0;
        }
        
        cpu_relax();
    }
}