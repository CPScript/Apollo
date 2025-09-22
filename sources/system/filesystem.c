#include "filesystem.h"
#include "heap_allocator.h"
#include "time_keeper.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    fs_file_info_t files[FS_MAX_FILES];
    uint8_t* data_blocks[FS_MAX_BLOCKS];
    bool block_allocated[FS_MAX_BLOCKS];
    uint32_t current_directory_id;
    bool is_initialized;
    uint32_t next_file_id;
    uint32_t system_time;
} filesystem_state_t;

static filesystem_state_t fs_state = {0};

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

static int string_compare(const char* str1, const char* str2) {
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return *str1 - *str2;
}

static void string_append(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static void memory_copy(void* dest, const void* src, uint32_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static void memory_set(void* dest, uint8_t value, uint32_t size) {
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < size; i++) {
        d[i] = value;
    }
}

static uint32_t get_current_time(void) {
    return ++fs_state.system_time;
}

static uint32_t allocate_block(void) {
    for (uint32_t i = 1; i < FS_MAX_BLOCKS; i++) {
        if (!fs_state.block_allocated[i]) {
            fs_state.block_allocated[i] = true;
            if (!fs_state.data_blocks[i]) {
                fs_state.data_blocks[i] = apollo_allocate_memory(FS_BLOCK_SIZE);
                if (!fs_state.data_blocks[i]) {
                    fs_state.block_allocated[i] = false;
                    return 0;
                }
                memory_set(fs_state.data_blocks[i], 0, FS_BLOCK_SIZE);
            }
            return i;
        }
    }
    return 0; 
}

static void free_block(uint32_t block_id) {
    if (block_id > 0 && block_id < FS_MAX_BLOCKS && fs_state.block_allocated[block_id]) {
        fs_state.block_allocated[block_id] = false;
        if (fs_state.data_blocks[block_id]) {
            memory_set(fs_state.data_blocks[block_id], 0, FS_BLOCK_SIZE);
        }
    }
}

static uint32_t allocate_file_id(void) {
    for (uint32_t i = 2; i < FS_MAX_FILES; i++) {
        if (!fs_state.files[i].is_valid) {
            return i;
        }
    }
    return 0;
}

static void extract_filename(const char* path, char* filename) {
    const char* last_slash = path;
    const char* current = path;
    
    while (*current) {
        if (*current == '/') {
            last_slash = current + 1;
        }
        current++;
    }
    
    string_copy(filename, last_slash);
}

static void extract_directory(const char* path, char* directory) {
    const char* last_slash = NULL;
    const char* current = path;
    
    while (*current) {
        if (*current == '/') {
            last_slash = current;
        }
        current++;
    }
    
    if (last_slash) {
        uint32_t len = last_slash - path;
        for (uint32_t i = 0; i < len && i < FS_MAX_PATH_LENGTH - 1; i++) {
            directory[i] = path[i];
        }
        directory[len] = '\0';
        if (len == 0) {
            string_copy(directory, "/");
        }
    } else {
        string_copy(directory, "/");
    }
}

static uint32_t find_file_in_directory(uint32_t dir_id, const char* name) {
    for (uint32_t i = 1; i < FS_MAX_FILES; i++) {
        if (fs_state.files[i].is_valid && 
            fs_state.files[i].parent_id == dir_id &&
            string_compare(fs_state.files[i].name, name) == 0) {
            return i;
        }
    }
    return 0;
}

static uint32_t resolve_path_to_id(const char* path) {
    if (!path || string_length(path) == 0) {
        return fs_state.current_directory_id;
    }
    
    uint32_t current_id;
    
    if (path[0] == '/') {
        current_id = 1;
        path++;
    } else {
        current_id = fs_state.current_directory_id;
    }
    
    if (string_length(path) == 0) {
        return current_id;
    }
    
    char component[FS_MAX_FILENAME_LENGTH];
    uint32_t comp_pos = 0;
    
    while (*path) {
        if (*path == '/') {
            if (comp_pos > 0) {
                component[comp_pos] = '\0';
                
                if (string_compare(component, ".") == 0) {
                } else if (string_compare(component, "..") == 0) {
                    if (current_id != 1) {
                        current_id = fs_state.files[current_id].parent_id;
                    }
                } else {
                    uint32_t found_id = find_file_in_directory(current_id, component);
                    if (found_id == 0) {
                        return 0;
                    }
                    current_id = found_id;
                }
                comp_pos = 0;
            }
        } else {
            if (comp_pos < FS_MAX_FILENAME_LENGTH - 1) {
                component[comp_pos++] = *path;
            }
        }
        path++;
    }
    
    if (comp_pos > 0) {
        component[comp_pos] = '\0';
        
        if (string_compare(component, ".") == 0) {
        } else if (string_compare(component, "..") == 0) {
            if (current_id != 1) {
                current_id = fs_state.files[current_id].parent_id;
            }
        } else {
            uint32_t found_id = find_file_in_directory(current_id, component);
            if (found_id == 0) {
                return 0;
            }
            current_id = found_id;
        }
    }
    
    return current_id;
}

static void write_file_content(uint32_t file_id, const char* content) {
    if (file_id == 0 || !fs_state.files[file_id].is_valid || 
        fs_state.files[file_id].type != FS_TYPE_FILE) {
        return;
    }
    
    uint32_t content_len = string_length(content);
    if (content_len > FS_BLOCK_SIZE) {
        content_len = FS_BLOCK_SIZE;
    }
    
    if (fs_state.files[file_id].data_block != 0) {
        memory_copy(fs_state.data_blocks[fs_state.files[file_id].data_block], 
                   content, content_len);
        fs_state.files[file_id].size = content_len;
        fs_state.files[file_id].modified_time = get_current_time();
    }
}

static void create_system_files(void) {
    uint32_t readme_id = find_file_in_directory(
        find_file_in_directory(1, "home"), "readme.txt");
    if (readme_id != 0) {
        const char* readme_content = 
            "Welcome to Apollo Operating System!\n\n"
            "This is a fully functional x86_64 kernel with:\n"
            "- Complete file system implementation\n"
            "- Text editor with real file I/O\n"
            "- Memory management\n"
            "- Process management\n"
            "- Hardware abstraction layer\n\n"
            "Commands to try:\n"
            "- ls        List files\n"
            "- cd        Change directory\n"
            "- cat       View file contents\n"
            "- edit      Edit files\n"
            "- mkdir     Create directories\n"
            "- touch     Create files\n"
            "- cp        Copy files\n"
            "- mv        Move files\n"
            "- find      Search files\n"
            "- grep      Search text\n"
            "- tree      Directory structure\n"
            "- help      All commands\n\n"
            "Apollo Kernel v1.0 - Built with modern C and Assembly\n";
        write_file_content(readme_id, readme_content);
    }
    
    uint32_t sample_id = find_file_in_directory(
        find_file_in_directory(1, "home"), "sample.c");
    if (sample_id != 0) {
        const char* sample_content = 
            "/*\n"
            " * Apollo Operating System\n"
            " */\n\n"
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n\n"
            "int main(void) {\n"
            "    printf(\"Hello from Apollo OS!\\n\");\n"
            "    printf(\"This kernel supports:\\n\");\n"
            "    printf(\"- Full file system\\n\");\n"
            "    printf(\"- Memory management\\n\");\n"
            "    printf(\"- Process management\\n\");\n"
            "    printf(\"- Hardware drivers\\n\");\n"
            "    \n"
            "    // Example of file operations\n"
            "    FILE* fp = fopen(\"/tmp/output.txt\", \"w\");\n"
            "    if (fp) {\n"
            "        fprintf(fp, \"File I/O works!\\n\");\n"
            "        fclose(fp);\n"
            "    }\n"
            "    \n"
            "    return 0;\n"
            "}\n\n"
            "/*\n"
            " * Compile with: gcc -o sample sample.c\n"
            " * Run with: ./sample\n"
            " */\n";
        write_file_content(sample_id, sample_content);
    }
    
    uint32_t config_id = find_file_in_directory(
        find_file_in_directory(1, "etc"), "config.cfg");
    if (config_id != 0) {
        const char* config_content = 
            "# Apollo Operating System Configuration\n"
            "# This file contains system configuration settings\n\n"
            "[system]\n"
            "kernel_version=1.0\n"
            "architecture=x86_64\n"
            "memory_model=paging\n"
            "scheduler=round_robin\n\n"
            "[filesystem]\n"
            "type=apollo_fs\n"
            "block_size=512\n"
            "max_files=256\n"
            "max_directories=64\n\n"
            "[display]\n"
            "mode=vga_text\n"
            "width=80\n"
            "height=25\n"
            "colors=16\n\n"
            "[input]\n"
            "keyboard=ps2\n"
            "mouse=disabled\n\n"
            "[network]\n"
            "enabled=false\n"
            "driver=none\n\n"
            "[debug]\n"
            "level=info\n"
            "serial_output=true\n"
            "log_file=/var/log/kernel.log\n";
        write_file_content(config_id, config_content);
    }
    
    uint32_t bin_dir = find_file_in_directory(1, "bin");
    if (bin_dir != 0) {
        if (filesystem_create_file("/bin/hello.sh")) {
            uint32_t script_id = find_file_in_directory(bin_dir, "hello.sh");
            if (script_id != 0) {
                const char* script_content = 
                    "#!/bin/sh\n"
                    "# Apollo OS Hello Script\n"
                    "echo \"Hello from Apollo Operating System!\"\n"
                    "echo \"Current directory: $(pwd)\"\n"
                    "echo \"Available commands:\"\n"
                    "ls /bin\n"
                    "echo \"System information:\"\n"
                    "sysinfo\n"
                    "echo \"File system usage:\"\n"
                    "df\n";
                write_file_content(script_id, script_content);
                fs_state.files[script_id].permissions |= FS_PERM_EXECUTE;
            }
        }
    }
    
    uint32_t dev_dir = find_file_in_directory(1, "dev");
    if (dev_dir != 0) {
        if (filesystem_create_file("/dev/version")) {
            uint32_t version_id = find_file_in_directory(dev_dir, "version");
            if (version_id != 0) {
                const char* version_content = 
                    "Apollo Operating System v1.0\n"
                    "Kernel Build: " __DATE__ " " __TIME__ "\n"
                    "Architecture: x86_64\n"
                    "Compiler: GCC " __VERSION__ "\n"
                    "Features: PAE, Long Mode, SSE, File System, Memory Management\n";
                write_file_content(version_id, version_content);
            }
        }
    }
    
    uint32_t tmp_dir = find_file_in_directory(1, "tmp");
    if (tmp_dir != 0) {
        if (filesystem_create_file("/tmp/notes.txt")) {
            uint32_t notes_id = find_file_in_directory(tmp_dir, "notes.txt");
            if (notes_id != 0) {
                const char* notes_content = 
                    "Apollo OS Development Notes\n"
                    "==========================\n\n"
                    "TODO List:\n"
                    "- [x] Basic kernel boot\n"
                    "- [x] Memory management\n"
                    "- [x] VGA text mode driver\n"
                    "- [x] PS/2 keyboard driver\n"
                    "- [x] File system implementation\n"
                    "- [x] Text editor\n"
                    "- [x] Shell commands\n"
                    "- [ ] Network stack\n"
                    "- [ ] GUI framework\n"
                    "- [ ] Audio driver\n\n"
                    "Performance Notes:\n"
                    "- Boot time: ~2 seconds\n"
                    "- Memory usage: ~2MB kernel\n"
                    "- File I/O: In-memory blocks\n"
                    "- Keyboard latency: <1ms\n\n"
                    "Architecture:\n"
                    "- Monolithic kernel design\n"
                    "- Modular component system\n"
                    "- Hardware abstraction layer\n"
                    "- Clean separation of concerns\n";
                write_file_content(notes_id, notes_content);
            }
        }
    }
}

void filesystem_initialize(void) {
    if (fs_state.is_initialized) return;
    
    for (uint32_t i = 0; i < FS_MAX_FILES; i++) {
        fs_state.files[i].is_valid = false;
    }
    
    for (uint32_t i = 0; i < FS_MAX_BLOCKS; i++) {
        fs_state.block_allocated[i] = false;
        fs_state.data_blocks[i] = NULL;
    }
    
    fs_state.system_time = 1000;
    
    fs_state.files[1].is_valid = true;
    string_copy(fs_state.files[1].name, "/");
    fs_state.files[1].type = FS_TYPE_DIRECTORY;
    fs_state.files[1].size = 0;
    fs_state.files[1].created_time = get_current_time();
    fs_state.files[1].modified_time = fs_state.files[1].created_time;
    fs_state.files[1].permissions = FS_PERM_READ | FS_PERM_WRITE | FS_PERM_EXECUTE;
    fs_state.files[1].parent_id = 1;
    fs_state.files[1].data_block = 0;
    
    fs_state.current_directory_id = 1;
    fs_state.next_file_id = 2;
    fs_state.is_initialized = true;
    
    filesystem_create_directory("/home");
    filesystem_create_directory("/bin");
    filesystem_create_directory("/etc");
    filesystem_create_directory("/tmp");
    filesystem_create_directory("/dev");
    filesystem_create_directory("/var");
    filesystem_create_directory("/var/log");
    filesystem_create_directory("/usr");
    filesystem_create_directory("/usr/bin");
    
    filesystem_create_file("/home/readme.txt");
    filesystem_create_file("/home/sample.c");
    filesystem_create_file("/etc/config.cfg");
    
    create_system_files();
}

bool filesystem_create_directory(const char* path) {
    if (!path || string_length(path) == 0) return false;
    
    char parent_path[FS_MAX_PATH_LENGTH];
    char dirname[FS_MAX_FILENAME_LENGTH];
    
    extract_directory(path, parent_path);
    extract_filename(path, dirname);
    
    uint32_t parent_id = resolve_path_to_id(parent_path);
    if (parent_id == 0 || fs_state.files[parent_id].type != FS_TYPE_DIRECTORY) {
        return false;
    }
    
    if (find_file_in_directory(parent_id, dirname) != 0) {
        return false;
    }
    
    uint32_t new_id = allocate_file_id();
    if (new_id == 0) return false;
    
    fs_state.files[new_id].is_valid = true;
    string_copy(fs_state.files[new_id].name, dirname);
    fs_state.files[new_id].type = FS_TYPE_DIRECTORY;
    fs_state.files[new_id].size = 0;
    fs_state.files[new_id].created_time = get_current_time();
    fs_state.files[new_id].modified_time = fs_state.files[new_id].created_time;
    fs_state.files[new_id].permissions = FS_PERM_READ | FS_PERM_WRITE | FS_PERM_EXECUTE;
    fs_state.files[new_id].parent_id = parent_id;
    fs_state.files[new_id].data_block = 0;
    
    return true;
}

bool filesystem_create_file(const char* path) {
    if (!path || string_length(path) == 0) return false;
    
    char parent_path[FS_MAX_PATH_LENGTH];
    char filename[FS_MAX_FILENAME_LENGTH];
    
    extract_directory(path, parent_path);
    extract_filename(path, filename);
    
    uint32_t parent_id = resolve_path_to_id(parent_path);
    if (parent_id == 0 || fs_state.files[parent_id].type != FS_TYPE_DIRECTORY) {
        return false;
    }
    
    if (find_file_in_directory(parent_id, filename) != 0) {
        return false;
    }
    
    uint32_t new_id = allocate_file_id();
    if (new_id == 0) return false;
    
    uint32_t block_id = allocate_block();
    if (block_id == 0) {
        return false;
    }
    
    fs_state.files[new_id].is_valid = true;
    string_copy(fs_state.files[new_id].name, filename);
    fs_state.files[new_id].type = FS_TYPE_FILE;
    fs_state.files[new_id].size = 0;
    fs_state.files[new_id].created_time = get_current_time();
    fs_state.files[new_id].modified_time = fs_state.files[new_id].created_time;
    fs_state.files[new_id].permissions = FS_PERM_READ | FS_PERM_WRITE;
    fs_state.files[new_id].parent_id = parent_id;
    fs_state.files[new_id].data_block = block_id;
    
    return true;
}

bool filesystem_delete_file(const char* path) {
    uint32_t file_id = resolve_path_to_id(path);
    if (file_id == 0 || file_id == 1) return false;
    
    if (fs_state.files[file_id].type == FS_TYPE_DIRECTORY) {
        for (uint32_t i = 1; i < FS_MAX_FILES; i++) {
            if (fs_state.files[i].is_valid && fs_state.files[i].parent_id == file_id) {
                return false;
            }
        }
    }
    
    if (fs_state.files[file_id].type == FS_TYPE_FILE && fs_state.files[file_id].data_block != 0) {
        free_block(fs_state.files[file_id].data_block);
    }
    
    fs_state.files[file_id].is_valid = false;
    
    return true;
}

bool filesystem_change_directory(const char* path) {
    uint32_t dir_id = resolve_path_to_id(path);
    if (dir_id == 0 || fs_state.files[dir_id].type != FS_TYPE_DIRECTORY) {
        return false;
    }
    
    fs_state.current_directory_id = dir_id;
    return true;
}

bool filesystem_get_current_directory(char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size == 0) return false;
    
    uint32_t current_id = fs_state.current_directory_id;
    
    if (current_id == 1) {
        if (buffer_size > 1) {
            string_copy(buffer, "/");
            return true;
        }
        return false;
    }
    
    char components[32][FS_MAX_FILENAME_LENGTH];
    uint32_t component_count = 0;
    
    while (current_id != 1 && component_count < 32) {
        string_copy(components[component_count], fs_state.files[current_id].name);
        component_count++;
        current_id = fs_state.files[current_id].parent_id;
    }
    
    buffer[0] = '\0';
    string_append(buffer, "/");
    
    for (int i = component_count - 1; i >= 0; i--) {
        if (string_length(buffer) + string_length(components[i]) + 1 < buffer_size) {
            string_append(buffer, components[i]);
            if (i > 0) {
                string_append(buffer, "/");
            }
        }
    }
    
    return true;
}

uint32_t filesystem_list_directory(const char* path, fs_dir_entry_t* entries, uint32_t max_entries) {
    uint32_t dir_id;
    
    if (path && string_length(path) > 0) {
        dir_id = resolve_path_to_id(path);
    } else {
        dir_id = fs_state.current_directory_id;
    }
    
    if (dir_id == 0 || fs_state.files[dir_id].type != FS_TYPE_DIRECTORY) {
        return 0;
    }
    
    uint32_t count = 0;
    
    for (uint32_t i = 1; i < FS_MAX_FILES && count < max_entries; i++) {
        if (fs_state.files[i].is_valid && fs_state.files[i].parent_id == dir_id) {
            string_copy(entries[count].name, fs_state.files[i].name);
            entries[count].type = fs_state.files[i].type;
            entries[count].size = fs_state.files[i].size;
            entries[count].permissions = fs_state.files[i].permissions;
            count++;
        }
    }
    
    return count;
}

bool filesystem_file_exists(const char* path) {
    return resolve_path_to_id(path) != 0;
}

bool filesystem_get_file_info(const char* path, fs_file_info_t* info) {
    uint32_t file_id = resolve_path_to_id(path);
    if (file_id == 0 || !info) return false;
    
    *info = fs_state.files[file_id];
    return true;
}

fs_file_handle_t* filesystem_open_file(const char* path, bool write_mode) {
    uint32_t file_id = resolve_path_to_id(path);
    if (file_id == 0 || fs_state.files[file_id].type != FS_TYPE_FILE) {
        return NULL;
    }
    
    fs_file_handle_t* handle = apollo_allocate_memory(sizeof(fs_file_handle_t));
    if (!handle) return NULL;
    
    handle->file_id = file_id;
    handle->position = 0;
    handle->is_open = true;
    handle->write_mode = write_mode;
    
    return handle;
}

void filesystem_close_file(fs_file_handle_t* handle) {
    if (handle) {
        handle->is_open = false;
        apollo_free_memory(handle);
    }
}

uint32_t filesystem_read_file(fs_file_handle_t* handle, void* buffer, uint32_t size) {
    if (!handle || !handle->is_open || !buffer) return 0;
    
    fs_file_info_t* file = &fs_state.files[handle->file_id];
    if (file->data_block == 0) return 0;
    
    uint32_t bytes_to_read = size;
    if (handle->position + bytes_to_read > file->size) {
        bytes_to_read = file->size - handle->position;
    }
    
    if (bytes_to_read == 0) return 0;
    
    uint8_t* src = fs_state.data_blocks[file->data_block] + handle->position;
    uint8_t* dest = (uint8_t*)buffer;
    
    memory_copy(dest, src, bytes_to_read);
    
    handle->position += bytes_to_read;
    return bytes_to_read;
}

uint32_t filesystem_write_file(fs_file_handle_t* handle, const void* buffer, uint32_t size) {
    if (!handle || !handle->is_open || !handle->write_mode || !buffer) return 0;
    
    fs_file_info_t* file = &fs_state.files[handle->file_id];
    if (file->data_block == 0) return 0;
    
    uint32_t bytes_to_write = size;
    if (handle->position + bytes_to_write > FS_BLOCK_SIZE) {
        bytes_to_write = FS_BLOCK_SIZE - handle->position;
    }
    
    if (bytes_to_write == 0) return 0;
    
    uint8_t* dest = fs_state.data_blocks[file->data_block] + handle->position;
    const uint8_t* src = (const uint8_t*)buffer;
    
    memory_copy(dest, src, bytes_to_write);
    
    handle->position += bytes_to_write;
    
    if (handle->position > file->size) {
        file->size = handle->position;
        file->modified_time = get_current_time();
    }
    
    return bytes_to_write;
}

bool filesystem_copy_file(const char* source, const char* destination) {
    uint32_t src_id = resolve_path_to_id(source);
    if (src_id == 0 || fs_state.files[src_id].type != FS_TYPE_FILE) {
        return false;
    }
    
    if (!filesystem_create_file(destination)) {
        return false;
    }
    
    fs_file_handle_t* src_handle = filesystem_open_file(source, false);
    fs_file_handle_t* dst_handle = filesystem_open_file(destination, true);
    
    if (!src_handle || !dst_handle) {
        if (src_handle) filesystem_close_file(src_handle);
        if (dst_handle) filesystem_close_file(dst_handle);
        return false;
    }
    
    uint8_t buffer[FS_BLOCK_SIZE];
    uint32_t bytes_read = filesystem_read_file(src_handle, buffer, FS_BLOCK_SIZE);
    if (bytes_read > 0) {
        filesystem_write_file(dst_handle, buffer, bytes_read);
    }
    
    filesystem_close_file(src_handle);
    filesystem_close_file(dst_handle);
    
    return true;
}

bool filesystem_move_file(const char* source, const char* destination) {
    if (!filesystem_copy_file(source, destination)) {
        return false;
    }
    
    return filesystem_delete_file(source);
}

uint32_t filesystem_get_free_space(void) {
    uint32_t free_blocks = 0;
    for (uint32_t i = 1; i < FS_MAX_BLOCKS; i++) {
        if (!fs_state.block_allocated[i]) {
            free_blocks++;
        }
    }
    return free_blocks * FS_BLOCK_SIZE;
}

uint32_t filesystem_get_used_space(void) {
    uint32_t used_blocks = 0;
    for (uint32_t i = 1; i < FS_MAX_BLOCKS; i++) {
        if (fs_state.block_allocated[i]) {
            used_blocks++;
        }
    }
    return used_blocks * FS_BLOCK_SIZE;
}

bool filesystem_get_stats(fs_stats_t* stats) {
    if (!stats) return false;
    
    stats->total_files = 0;
    stats->total_directories = 0;
    
    for (uint32_t i = 1; i < FS_MAX_FILES; i++) {
        if (fs_state.files[i].is_valid) {
            if (fs_state.files[i].type == FS_TYPE_FILE) {
                stats->total_files++;
            } else {
                stats->total_directories++;
            }
        }
    }
    
    stats->used_blocks = 0;
    for (uint32_t i = 1; i < FS_MAX_BLOCKS; i++) {
        if (fs_state.block_allocated[i]) {
            stats->used_blocks++;
        }
    }
    
    stats->free_blocks = (FS_MAX_BLOCKS - 1) - stats->used_blocks;
    stats->total_space = (FS_MAX_BLOCKS - 1) * FS_BLOCK_SIZE;
    stats->free_space = stats->free_blocks * FS_BLOCK_SIZE;
    
    return true;
}