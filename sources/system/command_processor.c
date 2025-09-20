#include "command_processor.h"
#include "terminal.h"
#include "input_manager.h"
#include "time_keeper.h"
#include "heap_allocator.h"
#include "text_editor.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGUMENTS 16
#define COMMAND_HISTORY_SIZE 32

typedef struct {
    char buffer[MAX_COMMAND_LENGTH];
    uint32_t length;
    uint32_t cursor_position;
} command_line_t;

typedef struct {
    char entries[COMMAND_HISTORY_SIZE][MAX_COMMAND_LENGTH];
    uint32_t count;
    uint32_t write_index;
    int32_t browse_index;
} command_history_t;

typedef struct {
    bool advanced_mode;
    bool initialized;
} shell_state_t;

static command_line_t current_command = {0};
static command_history_t history = {0};
static shell_state_t shell = {0};

static uint32_t string_length(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

static int string_compare(const char* str1, const char* str2) {
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

static void string_copy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static int string_to_integer(const char* str) {
    int result = 0;
    int sign = 1;
    
    if (*str == '-') {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

static uint32_t parse_arguments(const char* input, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    uint32_t arg_count = 0;
    uint32_t arg_pos = 0;
    bool in_argument = false;
    
    for (uint32_t i = 0; input[i] && arg_count < MAX_ARGUMENTS; i++) {
        if (input[i] == ' ' || input[i] == '\t') {
            if (in_argument) {
                args[arg_count][arg_pos] = '\0';
                arg_count++;
                arg_pos = 0;
                in_argument = false;
            }
        } else {
            if (!in_argument) {
                in_argument = true;
            }
            if (arg_pos < MAX_COMMAND_LENGTH - 1) {
                args[arg_count][arg_pos++] = input[i];
            }
        }
    }
    
    if (in_argument) {
        args[arg_count][arg_pos] = '\0';
        arg_count++;
    }
    
    return arg_count;
}

static void add_command_to_history(const char* command) {
    if (string_length(command) == 0) return;
    
    string_copy(history.entries[history.write_index], command);
    history.write_index = (history.write_index + 1) % COMMAND_HISTORY_SIZE;
    if (history.count < COMMAND_HISTORY_SIZE) {
        history.count++;
    }
    history.browse_index = -1;
}

static void browse_history(bool go_up) {
    if (history.count == 0) return;
    
    if (go_up) {
        if (history.browse_index == -1) {
            history.browse_index = 0;
        } else if (history.browse_index < (int32_t)history.count - 1) {
            history.browse_index++;
        }
    } else {
        if (history.browse_index > 0) {
            history.browse_index--;
        } else {
            history.browse_index = -1;
            current_command.length = 0;
            current_command.cursor_position = 0;
            return;
        }
    }
    
    while (current_command.cursor_position > 0) {
        terminal_backspace();
        current_command.cursor_position--;
    }
    
    uint32_t hist_idx = (history.write_index - 1 - history.browse_index + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;
    string_copy(current_command.buffer, history.entries[hist_idx]);
    current_command.length = string_length(current_command.buffer);
    current_command.cursor_position = current_command.length;
    
    for (uint32_t i = 0; i < current_command.length; i++) {
        terminal_write_char(current_command.buffer[i]);
    }
}

static void execute_help_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nApollo Shell Commands:\n");
    terminal_write_string("  help      - Show this help message\n");
    terminal_write_string("  clear     - Clear the screen\n");
    terminal_write_string("  echo      - Display text\n");
    terminal_write_string("  date      - Show current date and time\n");
    terminal_write_string("  uptime    - Show system uptime\n");
    terminal_write_string("  calc      - Basic calculator\n");
    terminal_write_string("  meminfo   - Display memory information\n");
    terminal_write_string("  sysinfo   - Show system information\n");
    terminal_write_string("  history   - Show command history\n");
    terminal_write_string("  palette   - Color palette demo\n");
    terminal_write_string("  edit      - Text editor (edit [filename])\n");
    terminal_write_string("  exit      - Exit shell mode\n");
    terminal_write_string("  reboot    - Restart the system\n");
    terminal_write_string("  shutdown  - Halt the system\n\n");
}

static void execute_clear_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_clear();
    if (shell.advanced_mode) {
        terminal_write_string("apollo-shell> ");
    } else {
        terminal_write_string("apollo> ");
    }
}

static void execute_echo_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    terminal_write_string("\n");
    for (uint32_t i = 1; i < argc; i++) {
        terminal_write_string(args[i]);
        if (i < argc - 1) terminal_write_string(" ");
    }
    terminal_write_string("\n");
}

static void execute_date_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    int year, month, day, hour, minute, second;
    time_keeper_get_datetime(&year, &month, &day, &hour, &minute, &second);
    
    terminal_write_string("\nCurrent Date/Time: ");
    terminal_write_int(year);
    terminal_write_string("-");
    if (month < 10) terminal_write_string("0");
    terminal_write_int(month);
    terminal_write_string("-");
    if (day < 10) terminal_write_string("0");
    terminal_write_int(day);
    terminal_write_string(" ");
    if (hour < 10) terminal_write_string("0");
    terminal_write_int(hour);
    terminal_write_string(":");
    if (minute < 10) terminal_write_string("0");
    terminal_write_int(minute);
    terminal_write_string(":");
    if (second < 10) terminal_write_string("0");
    terminal_write_int(second);
    terminal_write_string(" UTC\n");
}

static void execute_calc_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    if (argc != 4) {
        terminal_write_string("\nUsage: calc <num1> <op> <num2>\n");
        terminal_write_string("Operators: +, -, *, /\n");
        terminal_write_string("Example: calc 15 + 25\n");
        return;
    }
    
    int num1 = string_to_integer(args[1]);
    int num2 = string_to_integer(args[3]);
    char op = args[2][0];
    int result = 0;
    
    switch (op) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/':
            if (num2 == 0) {
                terminal_write_string("\nError: Division by zero\n");
                return;
            }
            result = num1 / num2;
            break;
        default:
            terminal_write_string("\nError: Invalid operator\n");
            return;
    }
    
    terminal_write_string("\nResult: ");
    terminal_write_int(num1);
    terminal_write_string(" ");
    terminal_write_char(op);
    terminal_write_string(" ");
    terminal_write_int(num2);
    terminal_write_string(" = ");
    terminal_write_int(result);
    terminal_write_string("\n");
}

static void execute_meminfo_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nMemory Information:\n");
    terminal_write_string("  Total RAM: 1 GB (simulated)\n");
    terminal_write_string("  Available: 512 MB\n");
    terminal_write_string("  Kernel Memory: 2 MB\n");
    terminal_write_string("  Page Size: 4 KB\n");
    terminal_write_string("  Memory Model: x86_64 Paging\n");
}

static void execute_sysinfo_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nSystem Information:\n");
    terminal_write_string("  OS: ApolloKernel v1.0\n");
    terminal_write_string("  Architecture: x86_64\n");
    terminal_write_string("  CPU: x86_64 Compatible\n");
    terminal_write_string("  Features: PAE, Long Mode, SSE\n");
    terminal_write_string("  Boot Protocol: Multiboot2\n");
    terminal_write_string("  Graphics: VGA Text Mode 80x25\n");
}

static void execute_history_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nCommand History:\n");
    for (uint32_t i = 0; i < history.count; i++) {
        uint32_t idx = (history.write_index - history.count + i + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;
        terminal_write_string("  ");
        terminal_write_uint(i + 1);
        terminal_write_string(": ");
        terminal_write_string(history.entries[idx]);
        terminal_write_string("\n");
    }
}

static void execute_palette_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nColor Palette Demo:\n\n");
    
    for (int bg = 0; bg < 16; bg++) {
        terminal_write_string("BG ");
        terminal_write_int(bg);
        terminal_write_string(": ");
        for (int fg = 0; fg < 16; fg++) {
            terminal_set_color(fg, bg);
            terminal_write_int(fg);
            terminal_write_char(' ');
        }
        terminal_set_color(7, 0); // Reset to light gray on black
        terminal_write_string("\n");
    }
    terminal_write_string("\n");
}

static void execute_edit_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    const char* filename = (argc > 1) ? args[1] : NULL;
    
    terminal_write_string("\nLaunching Apollo Text Editor...\n");
    
    for (volatile int i = 0; i < 1000000; i++) {
        __asm__ volatile("pause");
    }
    
    text_editor_initialize();
    
    text_editor_run(filename);
    
    terminal_write_string("Returned to shell.\n");
}

static void execute_shell_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    shell.advanced_mode = true;
    terminal_clear();
    terminal_write_string("Apollo Advanced Shell v1.0\n");
    terminal_write_string("Enhanced command environment active\n");
    terminal_write_string("Type 'help' for available commands\n\n");
    terminal_write_string("apollo-shell> ");
}

static void execute_exit_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    if (shell.advanced_mode) {
        shell.advanced_mode = false;
        terminal_clear();
        terminal_write_string("Exited shell mode\n");
        terminal_write_string("apollo> ");
    } else {
        terminal_write_string("\nUse 'shutdown' or 'reboot' to exit the system\n");
    }
}

static void execute_reboot_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nRebooting system...\n");
    __asm__ __volatile__("int $0x0");
}

static void execute_shutdown_command(uint32_t argc, char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH]) {
    (void)argc; (void)args;
    
    terminal_write_string("\nSystem halted. Safe to power off.\n");
    __asm__ __volatile__("cli; hlt");
}

static void execute_command(const char* input) {
    char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH];
    uint32_t argc = parse_arguments(input, args);
    
    if (argc == 0) return;
    
    add_command_to_history(input);
    
    if (string_compare(args[0], "help") == 0) {
        execute_help_command(argc, args);
    } else if (string_compare(args[0], "clear") == 0) {
        execute_clear_command(argc, args);
        return; // Don't print prompt again
    } else if (string_compare(args[0], "echo") == 0) {
        execute_echo_command(argc, args);
    } else if (string_compare(args[0], "date") == 0) {
        execute_date_command(argc, args);
    } else if (string_compare(args[0], "calc") == 0) {
        execute_calc_command(argc, args);
    } else if (string_compare(args[0], "meminfo") == 0) {
        execute_meminfo_command(argc, args);
    } else if (string_compare(args[0], "sysinfo") == 0) {
        execute_sysinfo_command(argc, args);
    } else if (string_compare(args[0], "history") == 0) {
        execute_history_command(argc, args);
    } else if (string_compare(args[0], "palette") == 0) {
        execute_palette_command(argc, args);
    } else if (string_compare(args[0], "edit") == 0) {
        execute_edit_command(argc, args);
        return; // Don't print prompt again since editor handles screen
    } else if (string_compare(args[0], "shell") == 0) {
        execute_shell_command(argc, args);
        return; // Don't print prompt again
    } else if (string_compare(args[0], "exit") == 0) {
        execute_exit_command(argc, args);
        return; // Don't print prompt again
    } else if (string_compare(args[0], "reboot") == 0) {
        execute_reboot_command(argc, args);
    } else if (string_compare(args[0], "shutdown") == 0) {
        execute_shutdown_command(argc, args);
    } else {
        terminal_write_string("\nUnknown command: ");
        terminal_write_string(args[0]);
        terminal_write_string("\nType 'help' for available commands\n");
    }
    
    if (shell.advanced_mode) {
        terminal_write_string("apollo-shell> ");
    } else {
        terminal_write_string("apollo> ");
    }
}

void command_processor_initialize(void) {
    shell.advanced_mode = false;
    shell.initialized = true;
    
    current_command.length = 0;
    current_command.cursor_position = 0;
    
    history.count = 0;
    history.write_index = 0;
    history.browse_index = -1;
    
    text_editor_initialize();
}

void command_processor_handle_input(uint8_t scan_code) {
    if (scan_code == SCANCODE_BACKSPACE) {
        if (current_command.cursor_position > 0) {
            current_command.cursor_position--;
            current_command.length--;
            terminal_backspace();
        }
    } else if (scan_code == SCANCODE_UP_ARROW) {
        browse_history(true);
    } else if (scan_code == SCANCODE_DOWN_ARROW) {
        browse_history(false);
    } else {
        char ascii_char = input_manager_scancode_to_ascii(scan_code);
        
        if (ascii_char == '\n' || ascii_char == '\r') {
            current_command.buffer[current_command.length] = '\0';
            terminal_write_string("\n");
            execute_command(current_command.buffer);
            current_command.length = 0;
            current_command.cursor_position = 0;
            history.browse_index = -1;
        } else if (ascii_char != 0 && current_command.length < MAX_COMMAND_LENGTH - 1) {
            current_command.buffer[current_command.cursor_position] = ascii_char;
            current_command.cursor_position++;
            current_command.length++;
            terminal_write_char(ascii_char);
        }
    }
}