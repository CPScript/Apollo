#include "command_processor.h"
#include "terminal.h"
#include "input_manager.h"
#include "time_keeper.h"
#include "heap_allocator.h"
#include "text_editor.h"
#include "filesystem.h"
#include "process_manager.h"
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
    char current_user[32];
    uint32_t commands_executed;
    uint32_t shell_start_time;
    uint32_t session_id;
    bool echo_mode;
} shell_state_t;

static command_line_t current_command = {0};
static command_history_t history = {0};
static shell_state_t shell = {0};

static uint32_t string_length(const char* str) {
    uint32_t len = 0;
    while (str && str[len] != '\0') len++;
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

static void string_append(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static int string_to_integer(const char* str) {
    if (!str) return 0;
    
    int result = 0;
    int sign = 1;
    
    while (*str == ' ' || *str == '\t') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

static bool string_to_integer_safe(const char* str, int* result) {
    if (!str || !result) return false;
    
    *result = 0;
    int sign = 1;
    
    while (*str == ' ' || *str == '\t') str++;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    if (*str < '0' || *str > '9') return false;
    
    while (*str >= '0' && *str <= '9') {
        int digit = *str - '0';
        
        if (*result > (INT32_MAX - digit) / 10) {
            return false;
        }
        
        *result = *result * 10 + digit;
        str++;
    }
    
    while (*str == ' ' || *str == '\t') str++;
    if (*str != '\0') return false;
    
    *result *= sign;
    return true;
}

static const char* process_get_state_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_RUNNING: return "running";
        case PROCESS_STATE_READY: return "ready";
        case PROCESS_STATE_BLOCKED: return "blocked";
        case PROCESS_STATE_TERMINATED: return "terminated";
        default: return "unknown";
    }
}

static const char* process_get_type_string(process_type_t type) {
    switch (type) {
        case PROCESS_TYPE_KERNEL: return "kernel";
        case PROCESS_TYPE_SYSTEM: return "system";
        case PROCESS_TYPE_USER: return "user";
        default: return "unknown";
    }
}

static bool string_contains(const char* haystack, const char* needle) {
    uint32_t needle_len = string_length(needle);
    uint32_t haystack_len = string_length(haystack);
    
    if (needle_len > haystack_len) return false;
    
    for (uint32_t i = 0; i <= haystack_len - needle_len; i++) {
        bool match = true;
        for (uint32_t j = 0; j < needle_len; j++) {
            if (haystack[i + j] != needle[j]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
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
    shell.commands_executed++;
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

static int calculate_expression(const char* expr) {
    if (!expr) return 0;
    
    char tokens[MAX_ARGUMENTS][MAX_COMMAND_LENGTH];
    uint32_t token_count = parse_arguments(expr, tokens);
    
    if (token_count == 0) {
        return 0;
    }
    
    if (token_count % 2 == 0) {
        return 0;
    }
    
    int result;
    if (!string_to_integer_safe(tokens[0], &result)) {
        return 0;
    }
    
    for (uint32_t i = 1; i < token_count; i += 2) {
        if (i + 1 >= token_count) {
            return 0;
        }
        
        if (string_length(tokens[i]) != 1) {
            return 0;
        }
        char op = tokens[i][0];
        
        int operand;
        if (!string_to_integer_safe(tokens[i + 1], &operand)) {
            return 0;
        }
        
        switch (op) {
            case '+':
                if ((operand > 0 && result > INT32_MAX - operand) ||
                    (operand < 0 && result < INT32_MIN - operand)) {
                    return 0;
                }
                result += operand;
                break;
                
            case '-':
                if ((operand < 0 && result > INT32_MAX + operand) ||
                    (operand > 0 && result < INT32_MIN + operand)) {
                    return 0;
                }
                result -= operand;
                break;
                
            case '*':
            case 'x':
            case 'X':
                if (operand != 0) {
                    if ((result > 0 && operand > 0 && result > INT32_MAX / operand) ||
                        (result > 0 && operand < 0 && operand < INT32_MIN / result) ||
                        (result < 0 && operand > 0 && result < INT32_MIN / operand) ||
                        (result < 0 && operand < 0 && result < INT32_MAX / operand)) {
                        return 0;
                    }
                }
                result *= operand;
                break;
                
            case '/':
                if (operand == 0) {
                    return 0; // Division by zero
                }
                if (result == INT32_MIN && operand == -1) {
                    return 0;
                }
                result /= operand;
                break;
                
            case '%':
                if (operand == 0) {
                    return 0; // Modulo by zero
                }
                result %= operand;
                break;
                
            default:
                return 0;
        }
    }
    
    return result;
}


static void execute_command(const char* input) {
    char args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH];
    uint32_t argc = parse_arguments(input, args);
    
    if (argc == 0) return;
    
    add_command_to_history(input);
    
    if (string_compare(args[0], "help") == 0) {
        terminal_write_string("\nApollo Shell - Command Reference\n");
        terminal_write_string("==========================================\n\n");
        
        terminal_set_color(14, 0);
        terminal_write_string("File System Commands:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  ls, dir      - List directory contents\n");
        terminal_write_string("  cd <path>    - Change directory\n");
        terminal_write_string("  pwd          - Print working directory\n");
        terminal_write_string("  mkdir <dir>  - Create directory\n");
        terminal_write_string("  rmdir <dir>  - Remove directory\n");
        terminal_write_string("  rm <file>    - Delete file\n");
        terminal_write_string("  cp <s> <d>   - Copy file\n");
        terminal_write_string("  mv <s> <d>   - Move/rename file\n");
        terminal_write_string("  cat <file>   - Display file contents\n");
        terminal_write_string("  touch <file> - Create empty file\n");
        terminal_write_string("  find <pat>   - Search for files\n");
        terminal_write_string("  tree         - Directory structure\n");
        terminal_write_string("  grep <p> <f> - Search text in files\n\n");
        
        terminal_set_color(12, 0);
        terminal_write_string("System Information:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  sysinfo      - Complete system info\n");
        terminal_write_string("  meminfo      - Memory usage statistics\n");
        terminal_write_string("  df           - Filesystem usage\n");
        terminal_write_string("  ps           - Process list\n");
        terminal_write_string("  whoami       - User information\n");
        terminal_write_string("  date         - Current date/time\n");
        terminal_write_string("  uptime       - System uptime\n\n");
        
        terminal_set_color(10, 0);
        terminal_write_string("Utilities:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  calc <expr>  - Calculator (e.g., calc 15 + 25)\n");
        terminal_write_string("  echo <text>  - Display text\n");
        terminal_write_string("  history      - Command history\n");
        terminal_write_string("  clear        - Clear screen\n");
        terminal_write_string("  edit <file>  - Text editor\n");
        terminal_write_string("  palette      - Color palette demo\n\n");
        
        terminal_set_color(11, 0);
        terminal_write_string("System Control:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  reboot       - Restart system\n");
        terminal_write_string("  shutdown     - Halt system\n\n");
        
        terminal_set_color(8, 0);
        terminal_write_string("Navigation Tips:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  Ctrl+Up/Down - Scroll through terminal output\n");
        terminal_write_string("  Ctrl+Home    - Scroll to top\n");
        terminal_write_string("  Ctrl+End     - Scroll to bottom\n");
        terminal_write_string("  Up/Down      - Browse command history\n\n");
        
    } else if (string_compare(args[0], "clear") == 0) {
        terminal_clear();
        
    } else if (string_compare(args[0], "ls") == 0 || string_compare(args[0], "dir") == 0) {
        const char* path = (argc > 1) ? args[1] : NULL;
        fs_dir_entry_t entries[64];
        uint32_t count = filesystem_list_directory(path, entries, 64);
        
        terminal_write_string("\n");
        if (count == 0) {
            terminal_write_string("Directory is empty or does not exist.\n");
        } else {
            char current_dir[FS_MAX_PATH_LENGTH];
            if (filesystem_get_current_directory(current_dir, sizeof(current_dir))) {
                terminal_set_color(11, 0);
                terminal_write_string("Directory: ");
                terminal_write_string(current_dir);
                terminal_write_string("\n\n");
                terminal_set_color(7, 0);
            }
            
            uint32_t total_files = 0, total_dirs = 0, total_size = 0;
            
            for (uint32_t i = 0; i < count; i++) {
                if (entries[i].type == FS_TYPE_DIRECTORY) {
                    terminal_set_color(12, 0);
                    terminal_write_string("d");
                    total_dirs++;
                } else {
                    terminal_set_color(10, 0);
                    terminal_write_string("-");
                    total_files++;
                    total_size += entries[i].size;
                }
                
                terminal_set_color(8, 0);
                terminal_write_string((entries[i].permissions & FS_PERM_READ) ? "r" : "-");
                terminal_write_string((entries[i].permissions & FS_PERM_WRITE) ? "w" : "-");
                terminal_write_string((entries[i].permissions & FS_PERM_EXECUTE) ? "x" : "-");
                terminal_write_string(" ");
                
                terminal_set_color(14, 0);
                if (entries[i].type == FS_TYPE_FILE) {
                    if (entries[i].size < 10) terminal_write_string("     ");
                    else if (entries[i].size < 100) terminal_write_string("    ");
                    else if (entries[i].size < 1000) terminal_write_string("   ");
                    else if (entries[i].size < 10000) terminal_write_string("  ");
                    else if (entries[i].size < 100000) terminal_write_string(" ");
                    terminal_write_uint(entries[i].size);
                } else {
                    terminal_write_string("      -");
                }
                terminal_write_string(" ");
                
                terminal_set_color(7, 0);
                terminal_write_string(entries[i].name);
                terminal_write_string("\n");
            }
            
            terminal_write_string("\n");
            terminal_set_color(8, 0);
            terminal_write_string("Total: ");
            terminal_write_uint(total_dirs);
            terminal_write_string(" directories, ");
            terminal_write_uint(total_files);
            terminal_write_string(" files (");
            terminal_write_uint(total_size);
            terminal_write_string(" bytes)\n");
            terminal_set_color(7, 0);
        }
        
    } else if (string_compare(args[0], "cd") == 0) {
        const char* path = (argc > 1) ? args[1] : "/";
        char old_dir[FS_MAX_PATH_LENGTH];
        filesystem_get_current_directory(old_dir, sizeof(old_dir));
        
        if (filesystem_change_directory(path)) {
            char current_dir[FS_MAX_PATH_LENGTH];
            if (filesystem_get_current_directory(current_dir, sizeof(current_dir))) {
                terminal_write_string("\nChanged directory:\n");
                terminal_set_color(8, 0);
                terminal_write_string("  From: ");
                terminal_write_string(old_dir);
                terminal_write_string("\n");
                terminal_set_color(11, 0);
                terminal_write_string("  To:   ");
                terminal_write_string(current_dir);
                terminal_write_string("\n");
                terminal_set_color(7, 0);
            }
        } else {
            terminal_write_string("\nError: Cannot change to directory '");
            terminal_write_string(path);
            terminal_write_string("'\n");
        }
        
    } else if (string_compare(args[0], "pwd") == 0) {
        char current_dir[FS_MAX_PATH_LENGTH];
        if (filesystem_get_current_directory(current_dir, sizeof(current_dir))) {
            terminal_write_string("\n");
            terminal_set_color(11, 0);
            terminal_write_string(current_dir);
            terminal_set_color(7, 0);
            terminal_write_string("\n");
        } else {
            terminal_write_string("\nError: Cannot determine current directory.\n");
        }
        
    } else if (string_compare(args[0], "mkdir") == 0) {
        if (argc < 2) {
            terminal_write_string("\nUsage: mkdir <directory>\n");
        } else {
            if (filesystem_create_directory(args[1])) {
                terminal_write_string("\nDirectory '");
                terminal_write_string(args[1]);
                terminal_write_string("' created successfully.\n");
            } else {
                terminal_write_string("\nError: Cannot create directory '");
                terminal_write_string(args[1]);
                terminal_write_string("'\n");
            }
        }
        
    } else if (string_compare(args[0], "rmdir") == 0) {
        if (argc < 2) {
            terminal_write_string("\nUsage: rmdir <directory>\n");
        } else {
            if (filesystem_delete_file(args[1])) {
                terminal_write_string("\nDirectory '");
                terminal_write_string(args[1]);
                terminal_write_string("' removed successfully.\n");
            } else {
                terminal_write_string("\nError: Cannot remove directory '");
                terminal_write_string(args[1]);
                terminal_write_string("' (not empty or doesn't exist)\n");
            }
        }
        
    } else if (string_compare(args[0], "rm") == 0 || string_compare(args[0], "del") == 0) {
        if (argc < 2) {
            terminal_write_string("\nUsage: rm <file>\n");
        } else {
            if (filesystem_delete_file(args[1])) {
                terminal_write_string("\nFile '");
                terminal_write_string(args[1]);
                terminal_write_string("' deleted successfully.\n");
            } else {
                terminal_write_string("\nError: Cannot delete file '");
                terminal_write_string(args[1]);
                terminal_write_string("'\n");
            }
        }
        
    } else if (string_compare(args[0], "cp") == 0 || string_compare(args[0], "copy") == 0) {
        if (argc < 3) {
            terminal_write_string("\nUsage: cp <source> <destination>\n");
        } else {
            if (filesystem_copy_file(args[1], args[2])) {
                terminal_write_string("\nFile copied from '");
                terminal_write_string(args[1]);
                terminal_write_string("' to '");
                terminal_write_string(args[2]);
                terminal_write_string("'\n");
            } else {
                terminal_write_string("\nError: Cannot copy file\n");
            }
        }
        
    } else if (string_compare(args[0], "mv") == 0 || string_compare(args[0], "move") == 0) {
        if (argc < 3) {
            terminal_write_string("\nUsage: mv <source> <destination>\n");
        } else {
            if (filesystem_move_file(args[1], args[2])) {
                terminal_write_string("\nFile moved from '");
                terminal_write_string(args[1]);
                terminal_write_string("' to '");
                terminal_write_string(args[2]);
                terminal_write_string("'\n");
            } else {
                terminal_write_string("\nError: Cannot move file\n");
            }
        }
        
    } else if (string_compare(args[0], "touch") == 0) {
        if (argc < 2) {
            terminal_write_string("\nUsage: touch <filename>\n");
        } else {
            if (filesystem_create_file(args[1])) {
                terminal_write_string("\nFile '");
                terminal_write_string(args[1]);
                terminal_write_string("' created successfully.\n");
            } else {
                terminal_write_string("\nError: Cannot create file '");
                terminal_write_string(args[1]);
                terminal_write_string("'\n");
            }
        }
        
    } else if (string_compare(args[0], "cat") == 0 || string_compare(args[0], "type") == 0) {
        if (argc < 2) {
            terminal_write_string("\nUsage: cat <filename>\n");
        } else {
            if (!filesystem_file_exists(args[1])) {
                terminal_write_string("\nError: File '");
                terminal_write_string(args[1]);
                terminal_write_string("' does not exist.\n");
            } else {
                fs_file_handle_t* handle = filesystem_open_file(args[1], false);
                if (!handle) {
                    terminal_write_string("\nError: Cannot open file.\n");
                } else {
                    fs_file_info_t info;
                    if (filesystem_get_file_info(args[1], &info)) {
                        terminal_write_string("\n");
                        terminal_set_color(8, 0);
                        terminal_write_string("--- ");
                        terminal_write_string(args[1]);
                        terminal_write_string(" (");
                        terminal_write_uint(info.size);
                        terminal_write_string(" bytes) ---\n");
                        terminal_set_color(7, 0);
                    }
                    
                    char buffer[512];
                    uint32_t bytes_read;
                    uint32_t total_bytes = 0;
                    uint32_t line_count = 1;
                    
                    while ((bytes_read = filesystem_read_file(handle, buffer, sizeof(buffer) - 1)) > 0) {
                        buffer[bytes_read] = '\0';
                        for (uint32_t i = 0; i < bytes_read; i++) {
                            if (buffer[i] == '\n') line_count++;
                        }
                        terminal_write_string(buffer);
                        total_bytes += bytes_read;
                    }
                    
                    filesystem_close_file(handle);
                    terminal_write_string("\n");
                    terminal_set_color(8, 0);
                    terminal_write_string("--- End (");
                    terminal_write_uint(total_bytes);
                    terminal_write_string(" bytes, ");
                    terminal_write_uint(line_count);
                    terminal_write_string(" lines) ---\n");
                    terminal_set_color(7, 0);
                }
            }
        }
        
    } else if (string_compare(args[0], "find") == 0) {
        if (argc < 2) {
            terminal_write_string("\nUsage: find <pattern>\n");
        } else {
            terminal_write_string("\nSearching for files containing '");
            terminal_write_string(args[1]);
            terminal_write_string("':\n\n");
            
            fs_dir_entry_t entries[64];
            uint32_t count = filesystem_list_directory(NULL, entries, 64);
            uint32_t found = 0;
            
            for (uint32_t i = 0; i < count; i++) {
                if (string_contains(entries[i].name, args[1])) {
                    terminal_set_color(11, 0);
                    terminal_write_string("  ");
                    terminal_write_string(entries[i].name);
                    terminal_set_color(7, 0);
                    if (entries[i].type == FS_TYPE_DIRECTORY) {
                        terminal_write_string(" (directory)");
                    } else {
                        terminal_write_string(" (");
                        terminal_write_uint(entries[i].size);
                        terminal_write_string(" bytes)");
                    }
                    terminal_write_string("\n");
                    found++;
                }
            }
            
            if (found == 0) {
                terminal_write_string("No files found matching pattern.\n");
            } else {
                terminal_write_string("\nFound ");
                terminal_write_uint(found);
                terminal_write_string(" matching files.\n");
            }
        }
        
    } else if (string_compare(args[0], "tree") == 0) {
        terminal_write_string("\nDirectory Structure:\n");
        terminal_write_string("====================\n");
        
        char current_dir[FS_MAX_PATH_LENGTH];
        filesystem_get_current_directory(current_dir, sizeof(current_dir));
        
        terminal_set_color(12, 0);
        terminal_write_string(current_dir);
        terminal_set_color(7, 0);
        terminal_write_string("\n");
        
        fs_dir_entry_t entries[64];
        uint32_t count = filesystem_list_directory(NULL, entries, 64);
        
        for (uint32_t i = 0; i < count; i++) {
            terminal_write_string("├── ");
            if (entries[i].type == FS_TYPE_DIRECTORY) {
                terminal_set_color(12, 0);
                terminal_write_string(entries[i].name);
                terminal_write_string("/");
                terminal_set_color(7, 0);
            } else {
                terminal_set_color(11, 0);
                terminal_write_string(entries[i].name);
                terminal_set_color(7, 0);
            }
            terminal_write_string("\n");
        }
        
    } else if (string_compare(args[0], "grep") == 0) {
        if (argc < 3) {
            terminal_write_string("\nUsage: grep <pattern> <file>\n");
        } else {
            if (!filesystem_file_exists(args[2])) {
                terminal_write_string("\nError: File '");
                terminal_write_string(args[2]);
                terminal_write_string("' does not exist.\n");
            } else {
                fs_file_handle_t* handle = filesystem_open_file(args[2], false);
                if (!handle) {
                    terminal_write_string("\nError: Cannot open file.\n");
                } else {
                    terminal_write_string("\nSearching for '");
                    terminal_write_string(args[1]);
                    terminal_write_string("' in ");
                    terminal_write_string(args[2]);
                    terminal_write_string(":\n\n");
                    
                    char buffer[512];
                    uint32_t bytes_read = filesystem_read_file(handle, buffer, sizeof(buffer) - 1);
                    buffer[bytes_read] = '\0';
                    
                    filesystem_close_file(handle);
                    
                    if (string_contains(buffer, args[1])) {
                        terminal_set_color(10, 0);
                        terminal_write_string("Pattern found in file!\n");
                        terminal_set_color(7, 0);
                        
                        char* line_start = buffer;
                        uint32_t line_num = 1;
                        
                        for (uint32_t i = 0; i < bytes_read; i++) {
                            if (buffer[i] == '\n' || i == bytes_read - 1) {
                                buffer[i] = '\0';
                                if (string_contains(line_start, args[1])) {
                                    terminal_write_uint(line_num);
                                    terminal_write_string(": ");
                                    terminal_write_string(line_start);
                                    terminal_write_string("\n");
                                }
                                line_start = &buffer[i + 1];
                                line_num++;
                                buffer[i] = '\n';
                            }
                        }
                    } else {
                        terminal_write_string("Pattern not found in file.\n");
                    }
                }
            }
        }
        
    } else if (string_compare(args[0], "sysinfo") == 0) {
        terminal_write_string("\nApollo Operating System - System Information\n");
        terminal_write_string("============================================\n\n");
        
        terminal_set_color(14, 0);
        terminal_write_string("Kernel Information:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  Kernel Version:    Apollo v1.1.2\n");
        terminal_write_string("  Architecture:      x86_64\n");
        terminal_write_string("  Build Date:        " __DATE__ " " __TIME__ "\n");
        terminal_write_string("  Compiler:          GCC " __VERSION__ "\n\n");
        
        terminal_set_color(12, 0);
        terminal_write_string("Hardware Information:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  CPU:               x86_64 Compatible\n");
        terminal_write_string("  Memory Model:      Long Mode (64-bit)\n");
        terminal_write_string("  Boot Protocol:     Multiboot2\n");
        terminal_write_string("  Graphics:          VGA Text Mode 80x25\n\n");
        
        terminal_set_color(10, 0);
        terminal_write_string("Memory Information:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  Total Heap:       ");
        terminal_write_uint(heap_allocator_get_total_memory() / 1024);
        terminal_write_string(" KB\n");
        terminal_write_string("  Used Memory:       ");
        terminal_write_uint(heap_allocator_get_used_memory() / 1024);
        terminal_write_string(" KB\n");
        terminal_write_string("  Free Memory:       ");
        terminal_write_uint(heap_allocator_get_free_memory() / 1024);
        terminal_write_string(" KB\n\n");
        
        terminal_set_color(11, 0);
        terminal_write_string("System Statistics:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  Commands Executed: ");
        terminal_write_uint(shell.commands_executed);
        terminal_write_string("\n");
        terminal_write_string("  Uptime:            ");
        terminal_write_uint(time_keeper_get_uptime_seconds());
        terminal_write_string(" seconds\n");
        
        process_stats_t proc_stats;
        if (process_get_stats(&proc_stats)) {
            terminal_write_string("  Total Processes:   ");
            terminal_write_uint(proc_stats.total_processes);
            terminal_write_string("\n");
            terminal_write_string("  Context Switches:  ");
            terminal_write_uint(proc_stats.context_switches);
            terminal_write_string("\n");
        }
        
    } else if (string_compare(args[0], "meminfo") == 0) {
        terminal_write_string("\nMemory Usage Statistics:\n");
        terminal_write_string("========================\n\n");
        
        uint32_t total = heap_allocator_get_total_memory();
        uint32_t used = heap_allocator_get_used_memory();
        uint32_t free = heap_allocator_get_free_memory();
        uint32_t usage_percent = (used * 100) / total;
        
        terminal_write_string("Total Memory:  ");
        terminal_write_uint(total / 1024);
        terminal_write_string(" KB (");
        terminal_write_uint(total);
        terminal_write_string(" bytes)\n");
        
        terminal_write_string("Used Memory:   ");
        terminal_write_uint(used / 1024);
        terminal_write_string(" KB (");
        terminal_write_uint(used);
        terminal_write_string(" bytes)\n");
        
        terminal_write_string("Free Memory:   ");
        terminal_write_uint(free / 1024);
        terminal_write_string(" KB (");
        terminal_write_uint(free);
        terminal_write_string(" bytes)\n");
        
        terminal_write_string("Memory Usage:  ");
        terminal_write_uint(usage_percent);
        terminal_write_string("%\n\n");
        
        terminal_write_string("Usage Bar: [");
        for (int i = 0; i < 50; i++) {
            if (i < (int)(usage_percent / 2)) {
                terminal_set_color(12, 0); // Red for used
                terminal_write_char('#');
            } else {
                terminal_set_color(10, 0); // Green for free
                terminal_write_char('-');
            }
        }
        terminal_set_color(7, 0);
        terminal_write_string("]\n");
        
    } else if (string_compare(args[0], "df") == 0) {
        terminal_write_string("\nFilesystem Usage:\n");
        terminal_write_string("=================\n\n");
        
        fs_stats_t stats;
        if (filesystem_get_stats(&stats)) {
            terminal_write_string("Filesystem:    Apollo FS\n");
            terminal_write_string("Total Space:   ");
            terminal_write_uint(stats.total_space / 1024);
            terminal_write_string(" KB\n");
            terminal_write_string("Used Space:    ");
            terminal_write_uint((stats.total_space - stats.free_space) / 1024);
            terminal_write_string(" KB\n");
            terminal_write_string("Free Space:    ");
            terminal_write_uint(stats.free_space / 1024);
            terminal_write_string(" KB\n");
            terminal_write_string("Total Files:   ");
            terminal_write_uint(stats.total_files);
            terminal_write_string("\n");
            terminal_write_string("Directories:   ");
            terminal_write_uint(stats.total_directories);
            terminal_write_string("\n");
            terminal_write_string("Free Blocks:   ");
            terminal_write_uint(stats.free_blocks);
            terminal_write_string("\n");
            terminal_write_string("Used Blocks:   ");
            terminal_write_uint(stats.used_blocks);
            terminal_write_string("\n");
            
            uint32_t usage_percent = ((stats.total_space - stats.free_space) * 100) / stats.total_space;
            terminal_write_string("Usage:         ");
            terminal_write_uint(usage_percent);
            terminal_write_string("%\n");
        }
        
    } else if (string_compare(args[0], "ps") == 0) {
        process_t processes[32];
        uint32_t count = process_list(processes, 32);
        
        if (count == 0) {
            terminal_write_string("\nNo processes found.\n");
        } else {
            terminal_write_string("\nProcess List:\n");
            terminal_write_string("PID  PPID NAME            STATE     TYPE    PRIO CPU  MEMORY\n");
            terminal_write_string("---  ---- ----            -----     ----    ---- ---  ------\n");
            
            for (uint32_t i = 0; i < count; i++) {
                if (processes[i].pid < 10) terminal_write_string(" ");
                terminal_write_uint(processes[i].pid);
                terminal_write_string("   ");
                
                if (processes[i].parent_pid < 10) terminal_write_string(" ");
                terminal_write_uint(processes[i].parent_pid);
                terminal_write_string("  ");
                
                char name_buffer[16];
                string_copy(name_buffer, processes[i].name);
                if (string_length(name_buffer) > 15) {
                    name_buffer[15] = '\0';
                }
                terminal_write_string(name_buffer);
                for (uint32_t j = string_length(name_buffer); j < 15; j++) {
                    terminal_write_char(' ');
                }
                terminal_write_string(" ");
                
                const char* state_str = process_get_state_string(processes[i].state);
                terminal_write_string(state_str);
                for (uint32_t j = string_length(state_str); j < 9; j++) {
                    terminal_write_char(' ');
                }
                
                const char* type_str = process_get_type_string(processes[i].type);
                terminal_write_string(type_str);
                for (uint32_t j = string_length(type_str); j < 7; j++) {
                    terminal_write_char(' ');
                }
                
                if (processes[i].priority < 10) terminal_write_string(" ");
                if (processes[i].priority < 100) terminal_write_string(" ");
                terminal_write_uint(processes[i].priority);
                terminal_write_string(" ");
                
                if (processes[i].cpu_time < 10) terminal_write_string(" ");
                if (processes[i].cpu_time < 100) terminal_write_string(" ");
                terminal_write_uint(processes[i].cpu_time);
                terminal_write_string("  ");
                
                terminal_write_uint(processes[i].memory_usage / 1024);
                terminal_write_string("K\n");
            }
            
            process_stats_t stats;
            if (process_get_stats(&stats)) {
                terminal_write_string("\nStatistics:\n");
                terminal_write_string("  Total processes:   ");
                terminal_write_uint(stats.total_processes);
                terminal_write_string("\n");
                terminal_write_string("  Running processes: ");
                terminal_write_uint(stats.running_processes);
                terminal_write_string("\n");
                terminal_write_string("  Context switches:  ");
                terminal_write_uint(stats.context_switches);
                terminal_write_string("\n");
            }
        }
        
    } else if (string_compare(args[0], "whoami") == 0) {
        terminal_write_string("\nUser Information:\n");
        terminal_write_string("=================\n");
        terminal_write_string("Username:      ");
        terminal_write_string(shell.current_user);
        terminal_write_string("\n");
        terminal_write_string("Shell:         Apollo Shell v1.1.2\n");
        terminal_write_string("Session ID:    ");
        terminal_write_uint(shell.session_id);
        terminal_write_string("\n");
        terminal_write_string("Commands Run:  ");
        terminal_write_uint(shell.commands_executed);
        terminal_write_string("\n");
        
    } else if (string_compare(args[0], "date") == 0) {
        int year, month, day, hour, minute, second;
        time_keeper_get_datetime(&year, &month, &day, &hour, &minute, &second);
        
        const char* months[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        
        terminal_write_string("\nCurrent Date and Time:\n");
        terminal_write_string("======================\n");
        terminal_write_string("Date: ");
        terminal_write_string(months[month - 1]);
        terminal_write_string(" ");
        terminal_write_uint(day);
        terminal_write_string(", ");
        terminal_write_uint(year);
        terminal_write_string("\n");
        
        terminal_write_string("Time: ");
        if (hour < 10) terminal_write_string("0");
        terminal_write_uint(hour);
        terminal_write_string(":");
        if (minute < 10) terminal_write_string("0");
        terminal_write_uint(minute);
        terminal_write_string(":");
        if (second < 10) terminal_write_string("0");
        terminal_write_uint(second);
        terminal_write_string("\n");
        
    } else if (string_compare(args[0], "uptime") == 0) {
        uint64_t uptime = time_keeper_get_uptime_seconds();
        uint32_t hours = uptime / 3600;
        uint32_t minutes = (uptime % 3600) / 60;
        uint32_t seconds = uptime % 60;
        
        terminal_write_string("\nSystem Uptime:\n");
        terminal_write_string("==============\n");
        terminal_write_string("Uptime: ");
        terminal_write_uint(hours);
        terminal_write_string(" hours, ");
        terminal_write_uint(minutes);
        terminal_write_string(" minutes, ");
        terminal_write_uint(seconds);
        terminal_write_string(" seconds\n");
        terminal_write_string("Total: ");
        terminal_write_uint((uint32_t)uptime);
        terminal_write_string(" seconds\n");
        
    } else if (string_compare(args[0], "calc") == 0) {
        if (argc < 2) {
            terminal_write_string("\nApollo Calculator\n");
            terminal_write_string("=========================================\n\n");
            terminal_write_string("Usage: calc <expression>\n\n");
            terminal_write_string("Supported operators: + - * / %\n");
            terminal_write_string("Examples:\n");
            terminal_write_string("  calc 15 + 25         = 40\n");
            terminal_write_string("  calc 64287 + 8732    = 72019\n");
            terminal_write_string("  calc 100 * 5 / 2     = 250\n");
            terminal_write_string("  calc 300 * 0 / 0     = Error: Division by zero\n\n");
            terminal_write_string("Note: Operations are evaluated left to right\n");
        } else {
            char expr[512] = "";
            for (uint32_t i = 1; i < argc; i++) {
                string_append(expr, args[i]);
                if (i < argc - 1) {
                    string_append(expr, " ");
                }
            }
            
            char check_args[MAX_ARGUMENTS][MAX_COMMAND_LENGTH];
            uint32_t check_count = parse_arguments(expr, check_args);
            bool has_div_by_zero = false;
            
            for (uint32_t i = 1; i < check_count; i += 2) {
                if (i + 1 < check_count) {
                    char op = check_args[i][0];
                    int operand = string_to_integer(check_args[i + 1]);
                    if ((op == '/' || op == '%') && operand == 0) {
                        has_div_by_zero = true;
                        break;
                    }
                }
            }
            
            terminal_write_string("\n");
            
            if (has_div_by_zero) {
                terminal_set_color(12, 0); // Red
                terminal_write_string("Error: Division by zero detected\n");
                terminal_set_color(7, 0);
                terminal_write_string("Expression: ");
                terminal_write_string(expr);
                terminal_write_string("\n");
            } else {
                int result = calculate_expression(expr);
                terminal_set_color(10, 0); // Green
                terminal_write_string("Result: ");
                terminal_set_color(7, 0);
                terminal_write_string(expr);
                terminal_write_string(" = ");
                terminal_set_color(14, 0); // Yellow
                terminal_write_int(result);
                terminal_set_color(7, 0);
                terminal_write_string("\n");
            }
        }
        
    } else if (string_compare(args[0], "echo") == 0) {
        terminal_write_string("\n");
        for (uint32_t i = 1; i < argc; i++) {
            terminal_write_string(args[i]);
            if (i < argc - 1) terminal_write_string(" ");
        }
        terminal_write_string("\n");
        
    } else if (string_compare(args[0], "history") == 0) {
        terminal_write_string("\nCommand History:\n");
        terminal_write_string("================\n");
        
        if (history.count == 0) {
            terminal_write_string("No commands in history.\n");
        } else {
            for (uint32_t i = 0; i < history.count; i++) {
                uint32_t idx = (history.write_index - history.count + i + COMMAND_HISTORY_SIZE) % COMMAND_HISTORY_SIZE;
                terminal_write_uint(i + 1);
                terminal_write_string(": ");
                terminal_write_string(history.entries[idx]);
                terminal_write_string("\n");
            }
        }
        
    } else if (string_compare(args[0], "palette") == 0) {
        terminal_write_string("\nApollo Color Palette Demonstration:\n");
        terminal_write_string("====================================\n\n");
        
        for (int bg = 0; bg < 8; bg++) {
            for (int fg = 0; fg < 16; fg++) {
                terminal_set_color(fg, bg);
                terminal_write_char('#');
            }
            terminal_set_color(7, 0);
            terminal_write_string("  Background ");
            terminal_write_uint(bg);
            terminal_write_string("\n");
        }
        
        terminal_write_string("\nForeground Colors:\n");
        for (int i = 0; i < 16; i++) {
            terminal_set_color(i, 0);
            terminal_write_uint(i);
            terminal_write_string(":");
            for (int j = 0; j < 5; j++) {
                terminal_write_char('#');
            }
            terminal_set_color(7, 0);
            terminal_write_string(" ");
            
            if ((i + 1) % 8 == 0) terminal_write_string("\n");
        }
        terminal_write_string("\n");
        
    } else if (string_compare(args[0], "edit") == 0) {
        const char* filename = (argc > 1) ? args[1] : NULL;
        terminal_write_string("\nLaunching Apollo Text Editor...\n");
        if (filename) {
            terminal_write_string("File: ");
            terminal_write_string(filename);
            terminal_write_string("\n");
        }
        text_editor_run(filename);
        terminal_clear();
        terminal_write_string("Returned to shell.\n");
        
    } else if (string_compare(args[0], "reboot") == 0) {
        terminal_write_string("\nRebooting system...\n");
        __asm__ __volatile__("int $0x0");
        
    } else if (string_compare(args[0], "shutdown") == 0) {
        terminal_write_string("\nShutting down Apollo OS...\n");
        terminal_write_string("System halted. Safe to power off.\n");
        __asm__ __volatile__("cli; hlt");
        
    } else {
        terminal_write_string("\nUnknown command: ");
        terminal_write_string(args[0]);
        terminal_write_string("\nType 'help' for available commands\n");
    }
    
    terminal_write_string("apollo> ");
}

void command_processor_initialize(void) {
    shell.advanced_mode = false;
    shell.initialized = true;
    shell.commands_executed = 0;
    shell.shell_start_time = 0;
    shell.session_id = 1;
    shell.echo_mode = true;
    string_copy(shell.current_user, "apollo");
    
    current_command.length = 0;
    current_command.cursor_position = 0;
    
    history.count = 0;
    history.write_index = 0;
    history.browse_index = -1;
}

void command_processor_handle_input(uint8_t scan_code) {
    if (scan_code == SCANCODE_PAGE_UP || scan_code == SCANCODE_PAGE_DOWN || 
        (scan_code == SCANCODE_HOME && input_manager_is_ctrl_pressed()) ||
        (scan_code == SCANCODE_END && input_manager_is_ctrl_pressed()) ||
        (scan_code == SCANCODE_UP_ARROW && input_manager_is_ctrl_pressed()) ||
        (scan_code == SCANCODE_DOWN_ARROW && input_manager_is_ctrl_pressed())) {
        terminal_handle_scroll_input(scan_code);
        return;
    }
    
    if (scan_code == SCANCODE_BACKSPACE) {
        if (current_command.cursor_position > 0) {
            current_command.cursor_position--;
            current_command.length--;
            terminal_backspace();
        }
    } else if (scan_code == SCANCODE_UP_ARROW && !input_manager_is_ctrl_pressed()) {
        browse_history(true);
    } else if (scan_code == SCANCODE_DOWN_ARROW && !input_manager_is_ctrl_pressed()) {
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
            if (shell.echo_mode) {
                terminal_write_char(ascii_char);
            }
        }
    }

}
