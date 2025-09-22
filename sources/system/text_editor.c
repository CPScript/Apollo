#include "text_editor.h"
#include "terminal.h"
#include "input_manager.h"
#include "filesystem.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char lines[TEXT_EDITOR_MAX_LINES][TEXT_EDITOR_MAX_LINE_LENGTH];
    uint32_t line_count;
    uint32_t cursor_line;
    uint32_t cursor_column;
    uint32_t top_line;
    char filename[TEXT_EDITOR_MAX_FILENAME];
    bool is_active;
    bool has_changes;
    bool insert_mode;
    bool file_loaded;
} text_editor_state_t;

static text_editor_state_t editor = {0};

static uint32_t string_length(const char* str) {
    uint32_t len = 0;
    while (str && str[len] != '\0') len++;
    return len;
}

static void string_copy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static void memory_set(void* dest, uint8_t value, uint32_t size) {
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = value;
    }
}

static void format_file_size(uint32_t size, char* buffer) {
    if (size < 1024) {
        uint32_t i = 0;
        uint32_t temp = size;
        if (temp == 0) {
            buffer[i++] = '0';
        } else {
            char digits[16];
            uint32_t digit_count = 0;
            while (temp > 0) {
                digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int j = digit_count - 1; j >= 0; j--) {
                buffer[i++] = digits[j];
            }
        }
        buffer[i++] = ' ';
        buffer[i++] = 'B';
        buffer[i] = '\0';
    } else {
        uint32_t kb = size / 1024;
        uint32_t i = 0;
        uint32_t temp = kb;
        if (temp == 0) {
            buffer[i++] = '0';
        } else {
            char digits[16];
            uint32_t digit_count = 0;
            while (temp > 0) {
                digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int j = digit_count - 1; j >= 0; j--) {
                buffer[i++] = digits[j];
            }
        }
        buffer[i++] = ' ';
        buffer[i++] = 'K';
        buffer[i++] = 'B';
        buffer[i] = '\0';
    }
}

static uint32_t get_file_size(void) {
    if (!editor.file_loaded || string_length(editor.filename) == 0) {
        return 0;
    }
    
    fs_file_info_t info;
    if (filesystem_get_file_info(editor.filename, &info)) {
        return info.size;
    }
    return 0;
}

static void draw_editor_screen(void) {
    terminal_clear();
    
    terminal_set_color(15, 1); // White on blue
    terminal_write_string("Apollo Text Editor - ");
    if (editor.filename[0] != '\0') {
        terminal_write_string(editor.filename);
    } else {
        terminal_write_string("Untitled");
    }
    if (editor.has_changes) {
        terminal_write_string(" [Modified]");
    }
    
    if (editor.file_loaded) {
        uint32_t file_size = get_file_size();
        char size_str[32];
        format_file_size(file_size, size_str);
        terminal_write_string(" (");
        terminal_write_string(size_str);
        terminal_write_string(")");
    }
    
    uint32_t title_length = 20 + string_length(editor.filename) + 
                           (editor.has_changes ? 11 : 0) +
                           (editor.file_loaded ? 20 : 0);
    for (uint32_t i = title_length; i < 80; i++) {
        terminal_write_char(' ');
    }
    
    terminal_set_color(7, 0); // Reset to normal
    terminal_write_string("\n");
    
    for (uint32_t screen_line = 0; screen_line < 22; screen_line++) {
        uint32_t text_line = editor.top_line + screen_line;
        
        if (text_line < editor.line_count) {
            terminal_set_color(8, 0); // Dark gray
            if (text_line + 1 < 10) terminal_write_char(' ');
            if (text_line + 1 < 100) terminal_write_char(' ');
            terminal_write_uint(text_line + 1);
            terminal_write_string(": ");
            terminal_set_color(7, 0); // Normal
            
            terminal_write_string(editor.lines[text_line]);
        } else if (text_line == 0 && editor.line_count == 0) {
            terminal_set_color(8, 0);
            terminal_write_string("    [Empty File - Start typing to add content]");
            terminal_set_color(7, 0);
        }
        
        if (screen_line < 21) terminal_write_string("\n");
    }
    
    terminal_set_color(0, 7); // Black on light gray
    
    terminal_write_string(" Line: ");
    terminal_write_uint(editor.cursor_line + 1);
    terminal_write_string("/");
    terminal_write_uint(editor.line_count);
    terminal_write_string(" Col: ");
    terminal_write_uint(editor.cursor_column + 1);
    
    terminal_write_string("  Mode: ");
    if (editor.insert_mode) {
        terminal_write_string("INSERT");
    } else {
        terminal_write_string("OVERWRITE");
    }
    
    terminal_write_string("  F1:Help F2:Save F3:Exit");
    
    for (uint32_t i = 55; i < 80; i++) {
        terminal_write_char(' ');
    }
    
    terminal_set_color(7, 0); // Reset
}

static void ensure_cursor_visible(void) {
    if (editor.cursor_line < editor.top_line) {
        editor.top_line = editor.cursor_line;
    }
    
    if (editor.cursor_line >= editor.top_line + 22) {
        editor.top_line = editor.cursor_line - 21;
    }
}

static void insert_character(char c) {
    if (editor.cursor_line >= TEXT_EDITOR_MAX_LINES) return;
    
    if (editor.cursor_line >= editor.line_count) {
        for (uint32_t i = editor.line_count; i <= editor.cursor_line; i++) {
            editor.lines[i][0] = '\0';
        }
        editor.line_count = editor.cursor_line + 1;
    }
    
    uint32_t line_len = string_length(editor.lines[editor.cursor_line]);
    
    if (editor.insert_mode) {
        if (line_len < TEXT_EDITOR_MAX_LINE_LENGTH - 1) {
            for (uint32_t i = line_len; i > editor.cursor_column; i--) {
                editor.lines[editor.cursor_line][i] = editor.lines[editor.cursor_line][i - 1];
            }
            editor.lines[editor.cursor_line][editor.cursor_column] = c;
            editor.lines[editor.cursor_line][line_len + 1] = '\0';
            editor.cursor_column++;
        }
    } else {
        if (editor.cursor_column < TEXT_EDITOR_MAX_LINE_LENGTH - 1) {
            editor.lines[editor.cursor_line][editor.cursor_column] = c;
            if (editor.cursor_column >= line_len) {
                editor.lines[editor.cursor_line][editor.cursor_column + 1] = '\0';
            }
            editor.cursor_column++;
        }
    }
    
    editor.has_changes = true;
}

static void delete_character(void) {
    if (editor.cursor_line >= editor.line_count) return;
    
    uint32_t line_len = string_length(editor.lines[editor.cursor_line]);
    
    if (editor.cursor_column < line_len) {
        for (uint32_t i = editor.cursor_column; i < line_len; i++) {
            editor.lines[editor.cursor_line][i] = editor.lines[editor.cursor_line][i + 1];
        }
        editor.has_changes = true;
    }
}

static void insert_newline(void) {
    if (editor.line_count >= TEXT_EDITOR_MAX_LINES - 1) return;
    
    if (editor.cursor_column < string_length(editor.lines[editor.cursor_line])) {
        for (uint32_t i = editor.line_count; i > editor.cursor_line; i--) {
            string_copy(editor.lines[i + 1], editor.lines[i]);
        }
        
        string_copy(editor.lines[editor.cursor_line + 1], 
                   &editor.lines[editor.cursor_line][editor.cursor_column]);
        editor.lines[editor.cursor_line][editor.cursor_column] = '\0';
        
        editor.line_count++;
    } else {
        for (uint32_t i = editor.line_count; i > editor.cursor_line; i--) {
            string_copy(editor.lines[i + 1], editor.lines[i]);
        }
        editor.lines[editor.cursor_line + 1][0] = '\0';
        editor.line_count++;
    }
    
    editor.cursor_line++;
    editor.cursor_column = 0;
    editor.has_changes = true;
}

static void move_cursor_up(void) {
    if (editor.cursor_line > 0) {
        editor.cursor_line--;
        uint32_t line_len = string_length(editor.lines[editor.cursor_line]);
        if (editor.cursor_column > line_len) {
            editor.cursor_column = line_len;
        }
    }
}

static void move_cursor_down(void) {
    if (editor.cursor_line < editor.line_count - 1) {
        editor.cursor_line++;
        uint32_t line_len = string_length(editor.lines[editor.cursor_line]);
        if (editor.cursor_column > line_len) {
            editor.cursor_column = line_len;
        }
    }
}

static void move_cursor_left(void) {
    if (editor.cursor_column > 0) {
        editor.cursor_column--;
    }
}

static void move_cursor_right(void) {
    uint32_t line_len = string_length(editor.lines[editor.cursor_line]);
    if (editor.cursor_column < line_len && editor.cursor_column < TEXT_EDITOR_MAX_LINE_LENGTH - 1) {
        editor.cursor_column++;
    }
}

static void show_help(void) {
    while (input_manager_has_input()) {
        input_manager_read_scancode();
    }
    
    terminal_clear();
    terminal_set_color(14, 0); // Yellow
    terminal_write_string("Apollo Text Editor - Help\n");
    terminal_write_string("==========================\n\n");
    terminal_set_color(7, 0);
    
    terminal_write_string("Do you want to view the help information? (y/n): ");
    
    bool show_full_help = false;
    while (true) {
        while (!input_manager_has_input()) {
            __asm__ volatile("pause");
        }
        uint8_t response_scan = input_manager_read_scancode();
        char response = input_manager_scancode_to_ascii(response_scan);
        
        if (response == 'y' || response == 'Y') {
            terminal_write_string("Yes\n\n");
            show_full_help = true;
            break;
        } else if (response == 'n' || response == 'N') {
            terminal_write_string("No\n");
            return; // Return to editor immediately
        } else if (response == 27 || response_scan == 0x01) { // Escape
            terminal_write_string("Cancelled\n");
            return; // Return to editor
        }
    }
    
    if (show_full_help) {
        terminal_set_color(11, 0); // Light cyan
        terminal_write_string("Navigation:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  Arrow Keys    - Move cursor around the text\n");
        terminal_write_string("  Home          - Jump to beginning of current line\n");
        terminal_write_string("  End           - Jump to end of current line\n");
        terminal_write_string("  Page Up/Down  - Scroll up/down by 10 lines\n\n");
        
        terminal_set_color(12, 0); // Light red
        terminal_write_string("Editing Commands:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  Insert        - Toggle between INSERT and OVERWRITE modes\n");
        terminal_write_string("  Delete        - Delete character at cursor position\n");
        terminal_write_string("  Backspace     - Delete character before cursor\n");
        terminal_write_string("  Enter         - Insert new line and move cursor down\n");
        terminal_write_string("  Tab           - Insert 4 spaces for indentation\n\n");
        
        terminal_set_color(10, 0); // Light green
        terminal_write_string("File Operations:\n");
        terminal_set_color(7, 0);
        terminal_write_string("  F1            - Show this help screen\n");
        terminal_write_string("  F2            - Save current file to Apollo filesystem\n");
        terminal_write_string("  F3            - Exit editor (prompts to save if modified)\n");
        terminal_write_string("  Escape        - Cancel current operation\n\n");
        
        while (input_manager_has_input()) {
            input_manager_read_scancode();
        }
        
        terminal_set_color(15, 4); // White on red for emphasis
        terminal_write_string(" Press any key to return to editing... ");
        terminal_set_color(7, 0);
        
        while (!input_manager_has_input()) {
            __asm__ volatile("pause");
        }
        input_manager_read_scancode();
        
        while (input_manager_has_input()) {
            input_manager_read_scancode();
        }
    }
}

static bool save_file(void) {
    if (string_length(editor.filename) == 0) {
        terminal_clear();
        terminal_set_color(12, 0); // Red
        terminal_write_string("Error: No filename specified.\n");
        terminal_set_color(7, 0);
        terminal_write_string("Press any key to continue...");
        while (!input_manager_has_input()) {
            __asm__ volatile("pause");
        }
        input_manager_read_scancode();
        return false;
    }
    
    fs_file_handle_t* handle = filesystem_open_file(editor.filename, true);
    if (!handle) {
        if (!filesystem_create_file(editor.filename)) {
            terminal_clear();
            terminal_set_color(12, 0); // Red
            terminal_write_string("Error: Cannot create file ");
            terminal_write_string(editor.filename);
            terminal_write_string("\n");
            terminal_set_color(7, 0);
            terminal_write_string("Press any key to continue...");
            while (!input_manager_has_input()) {
                __asm__ volatile("pause");
            }
            input_manager_read_scancode();
            return false;
        }
        
        handle = filesystem_open_file(editor.filename, true);
        if (!handle) {
            terminal_clear();
            terminal_set_color(12, 0); // Red
            terminal_write_string("Error: Cannot open file for writing\n");
            terminal_set_color(7, 0);
            terminal_write_string("Press any key to continue...");
            while (!input_manager_has_input()) {
                __asm__ volatile("pause");
            }
            input_manager_read_scancode();
            return false;
        }
    }
    
    for (uint32_t i = 0; i < editor.line_count; i++) {
        uint32_t line_len = string_length(editor.lines[i]);
        if (line_len > 0) {
            filesystem_write_file(handle, editor.lines[i], line_len);
        }
        if (i < editor.line_count - 1) {
            filesystem_write_file(handle, "\n", 1);
        }
    }
    
    filesystem_close_file(handle);
    
    terminal_clear();
    terminal_set_color(10, 0); // Green
    terminal_write_string("File saved successfully to ");
    terminal_write_string(editor.filename);
    terminal_write_string("\n");
    
    fs_file_info_t info;
    if (filesystem_get_file_info(editor.filename, &info)) {
        terminal_write_string("File size: ");
        terminal_write_uint(info.size);
        terminal_write_string(" bytes\n");
        terminal_write_string("Lines: ");
        terminal_write_uint(editor.line_count);
        terminal_write_string("\n");
    }
    
    terminal_set_color(7, 0);
    terminal_write_string("Press any key to continue...");
    
    editor.has_changes = false;
    
    while (!input_manager_has_input()) {
        __asm__ volatile("pause");
    }
    input_manager_read_scancode();
    
    return true;
}

static void handle_editor_input(uint8_t scan_code) {
    if (scan_code == 0x3B) { // F1 - Help
        show_help();
        return;
    } else if (scan_code == 0x3C) { // F2 - Save
        save_file();
        return;
    } else if (scan_code == 0x3D) { // F3 - Exit
        if (editor.has_changes) {
            terminal_clear();
            terminal_set_color(14, 0); // Yellow
            terminal_write_string("File has unsaved changes.\n");
            terminal_write_string("Save before exit? (y/n/ESC to cancel): ");
            terminal_set_color(7, 0);
            
            while (true) {
                while (!input_manager_has_input()) {
                    __asm__ volatile("pause");
                }
                uint8_t response_scan = input_manager_read_scancode();
                char response = input_manager_scancode_to_ascii(response_scan);
                
                if (response == 'y' || response == 'Y') {
                    terminal_write_string("Yes\n");
                    if (save_file()) {
                        editor.is_active = false;
                    }
                    return;
                } else if (response == 'n' || response == 'N') {
                    terminal_write_string("No\n");
                    editor.is_active = false;
                    return;
                } else if (response == 27 || response_scan == 0x01) { // Escape
                    terminal_write_string("Cancelled\n");
                    return; // Cancel exit
                }
            }
        } else {
            editor.is_active = false;
        }
        return;
    }
    
    uint8_t ascii = (uint8_t)input_manager_scancode_to_ascii(scan_code);
    
    switch (ascii) {
        case SCANCODE_UP_ARROW:
            move_cursor_up();
            break;
        case SCANCODE_DOWN_ARROW:
            move_cursor_down();
            break;
        case SCANCODE_LEFT_ARROW:
            move_cursor_left();
            break;
        case SCANCODE_RIGHT_ARROW:
            move_cursor_right();
            break;
        case SCANCODE_HOME:
            editor.cursor_column = 0;
            break;
        case SCANCODE_END:
            editor.cursor_column = string_length(editor.lines[editor.cursor_line]);
            break;
        case SCANCODE_PAGE_UP:
            for (int i = 0; i < 10; i++) move_cursor_up();
            break;
        case SCANCODE_PAGE_DOWN:
            for (int i = 0; i < 10; i++) move_cursor_down();
            break;
        case SCANCODE_INSERT:
            editor.insert_mode = !editor.insert_mode;
            break;
        case SCANCODE_DELETE:
            delete_character();
            break;
        case '\b': // Backspace
            if (editor.cursor_column > 0) {
                editor.cursor_column--;
                delete_character();
            }
            break;
        case '\n': // Enter
        case '\r': // Carriage return
            insert_newline();
            break;
        case '\t': // Tab
            for (int i = 0; i < 4; i++) {
                insert_character(' ');
            }
            break;
        case 27: 
            // just ignore escape
            break;
        default:
            if (ascii >= 32 && ascii <= 126) {
                insert_character((char)ascii);
            }
            break;
    }
    
    ensure_cursor_visible();
}

void text_editor_initialize(void) {
    editor.line_count = 0;
    editor.cursor_line = 0;
    editor.cursor_column = 0;
    editor.top_line = 0;
    editor.filename[0] = '\0';
    editor.is_active = false;
    editor.has_changes = false;
    editor.insert_mode = true;
    editor.file_loaded = false;
}

void text_editor_run(const char* filename) {
    editor.is_active = true;
    editor.insert_mode = true;
    editor.file_loaded = false;
    
    if (filename && string_length(filename) > 0) {
        string_copy(editor.filename, filename);
        text_editor_load_file(filename);
    } else {
        text_editor_new_file();
    }
    
    while (editor.is_active) {
        draw_editor_screen();
        
        while (!input_manager_has_input() && editor.is_active) {
            __asm__ volatile("pause");
        }
        
        if (input_manager_has_input()) {
            uint8_t scan_code = input_manager_read_scancode();
            handle_editor_input(scan_code);
        }
    }
    
    terminal_clear();
}

bool text_editor_load_file(const char* filename) {
    if (!filename || string_length(filename) == 0) {
        return false;
    }
    
    if (!filesystem_file_exists(filename)) {
        text_editor_new_file();
        string_copy(editor.filename, filename);
        return true;
    }
    
    fs_file_handle_t* handle = filesystem_open_file(filename, false);
    if (!handle) {
        return false;
    }
    
    text_editor_new_file();
    
    char buffer[FS_BLOCK_SIZE];
    uint32_t bytes_read = filesystem_read_file(handle, buffer, FS_BLOCK_SIZE - 1);
    buffer[bytes_read] = '\0';
    
    filesystem_close_file(handle);
    
    uint32_t line_pos = 0;
    uint32_t char_pos = 0;
    
    for (uint32_t i = 0; i < bytes_read && line_pos < TEXT_EDITOR_MAX_LINES; i++) {
        if (buffer[i] == '\n') {
            editor.lines[line_pos][char_pos] = '\0';
            line_pos++;
            char_pos = 0;
        } else if (char_pos < TEXT_EDITOR_MAX_LINE_LENGTH - 1) {
            editor.lines[line_pos][char_pos++] = buffer[i];
        }
    }
    
    if (char_pos > 0 || line_pos == 0) {
        editor.lines[line_pos][char_pos] = '\0';
        line_pos++;
    }
    
    editor.line_count = line_pos;
    editor.has_changes = false;
    editor.file_loaded = true;
    string_copy(editor.filename, filename);
    
    return true;
}

bool text_editor_save_file(const char* filename) {
    if (filename) {
        string_copy(editor.filename, filename);
    }
    return save_file();
}

void text_editor_new_file(void) {
    for (uint32_t i = 0; i < TEXT_EDITOR_MAX_LINES; i++) {
        editor.lines[i][0] = '\0';
    }
    editor.line_count = 1;
    editor.cursor_line = 0;
    editor.cursor_column = 0;
    editor.top_line = 0;
    editor.has_changes = false;
    editor.file_loaded = false;
}

bool text_editor_is_active(void) {
    return editor.is_active;
}

bool text_editor_has_unsaved_changes(void) {
    return editor.has_changes;
}