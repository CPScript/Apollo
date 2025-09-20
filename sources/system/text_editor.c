#include "text_editor.h"
#include "terminal.h"
#include "input_manager.h"
#include "heap_allocator.h"
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
} text_editor_state_t;

static text_editor_state_t editor = {0};

static uint32_t string_length(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

static void string_copy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
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
    
    uint32_t title_length = 20 + string_length(editor.filename) + (editor.has_changes ? 11 : 0);
    for (uint32_t i = title_length; i < 80; i++) {
        terminal_write_char(' ');
    }
    
    terminal_set_color(7, 0); // Reset to normal
    terminal_write_string("\n");
    
    for (uint32_t screen_line = 0; screen_line < 22; screen_line++) {
        uint32_t text_line = editor.top_line + screen_line;
        
        if (text_line < editor.line_count) {
            terminal_set_color(8, 0); // Dark gray
            if (text_line < 9) terminal_write_char(' ');
            terminal_write_uint(text_line + 1);
            terminal_write_string(": ");
            terminal_set_color(7, 0); // Normal
            
            terminal_write_string(editor.lines[text_line]);
        } else if (text_line == editor.line_count && screen_line == 0 && editor.line_count == 0) {
            terminal_set_color(8, 0);
            terminal_write_string("   [Empty File]");
            terminal_set_color(7, 0);
        }
        
        if (screen_line < 21) terminal_write_string("\n");
    }
    
    terminal_set_color(0, 7); // Black on light gray
    
    terminal_write_string("Line: ");
    terminal_write_uint(editor.cursor_line + 1);
    terminal_write_string(" Col: ");
    terminal_write_uint(editor.cursor_column + 1);
    
    terminal_write_string("  Mode: ");
    if (editor.insert_mode) {
        terminal_write_string("INSERT");
    } else {
        terminal_write_string("OVERWRITE");
    }
    
    terminal_write_string("  F1:Help F2:Save F3:Exit");
    
    for (uint32_t i = 45; i < 80; i++) {
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
    terminal_clear();
    terminal_set_color(14, 0); // Yellow
    terminal_write_string("Apollo Text Editor - Help\n\n");
    terminal_set_color(7, 0);
    
    terminal_write_string("Navigation:\n");
    terminal_write_string("  Arrow Keys    - Move cursor\n");
    terminal_write_string("  Home          - Beginning of line\n");
    terminal_write_string("  End           - End of line\n");
    terminal_write_string("  Page Up/Down  - Scroll page\n\n");
    
    terminal_write_string("Editing:\n");
    terminal_write_string("  Insert        - Toggle insert/overwrite mode\n");
    terminal_write_string("  Delete        - Delete character at cursor\n");
    terminal_write_string("  Backspace     - Delete character before cursor\n");
    terminal_write_string("  Enter         - Insert new line\n\n");
    
    terminal_write_string("File Operations:\n");
    terminal_write_string("  F2            - Save file\n");
    terminal_write_string("  F3            - Exit editor\n\n");
    
    terminal_write_string("Press any key to return to editing...");
    
    while (!input_manager_has_input()) {
        __asm__ volatile("pause");
    }
    input_manager_read_scancode();
}

static bool save_file(void) {
    // we'll just simulate saving since this is still under development
    terminal_clear();
    terminal_set_color(10, 0); // Green
    terminal_write_string("File saved successfully!\n");
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
    char ascii = input_manager_scancode_to_ascii(scan_code);
    
    if (ascii == SCANCODE_F1) {
        show_help();
        return;
    } else if (ascii == SCANCODE_F2) {
        save_file();
        return;
    } else if (ascii == SCANCODE_F3) {
        editor.is_active = false;
        return;
    }
    
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
            insert_newline();
            break;
        case '\t': // Tab
            for (int i = 0; i < 4; i++) {
                insert_character(' ');
            }
            break;
        default:
            if (ascii >= 32 && ascii <= 126) {
                insert_character(ascii);
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
}

void text_editor_run(const char* filename) {
    editor.is_active = true;
    editor.insert_mode = true;
    
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
    // This would read from filesystem but for now, create a sample file
    text_editor_new_file();
    
    string_copy(editor.lines[0], "// Welcome to Apollo Text Editor!");
    string_copy(editor.lines[1], "// This is a sample file.");
    string_copy(editor.lines[2], "");
    string_copy(editor.lines[3], "#include <stdio.h>");
    string_copy(editor.lines[4], "");
    string_copy(editor.lines[5], "int main() {");
    string_copy(editor.lines[6], "    printf(\"Hello, Apollo!\\n\");");
    string_copy(editor.lines[7], "    return 0;");
    string_copy(editor.lines[8], "}");
    
    editor.line_count = 9;
    editor.has_changes = false;
    
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
}

bool text_editor_is_active(void) {
    return editor.is_active;
}

bool text_editor_has_unsaved_changes(void) {
    return editor.has_changes;
}