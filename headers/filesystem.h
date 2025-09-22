#ifndef APOLLO_FILESYSTEM_H
#define APOLLO_FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>

#define FS_MAX_FILENAME_LENGTH 64
#define FS_MAX_PATH_LENGTH 256
#define FS_MAX_FILE_SIZE (64 * 1024)     // 64KB max file size
#define FS_MAX_FILES 256
#define FS_MAX_DIRECTORIES 64
#define FS_BLOCK_SIZE 512
#define FS_MAX_BLOCKS 1024

typedef enum {
    FS_TYPE_FILE = 1,
    FS_TYPE_DIRECTORY = 2
} fs_file_type_t;

typedef enum {
    FS_PERM_READ = 0x01,
    FS_PERM_WRITE = 0x02,
    FS_PERM_EXECUTE = 0x04
} fs_permissions_t;

typedef struct {
    char name[FS_MAX_FILENAME_LENGTH];
    fs_file_type_t type;
    uint32_t size;
    uint32_t created_time;
    uint32_t modified_time;
    uint8_t permissions;
    uint32_t parent_id;
    uint32_t data_block;
    bool is_valid;
} fs_file_info_t;

typedef struct {
    char name[FS_MAX_FILENAME_LENGTH];
    fs_file_type_t type;
    uint32_t size;
    uint8_t permissions;
} fs_dir_entry_t;

typedef struct {
    uint32_t file_id;
    uint32_t position;
    bool is_open;
    bool write_mode;
} fs_file_handle_t;

void filesystem_initialize(void);

bool filesystem_create_directory(const char* path);
bool filesystem_remove_directory(const char* path);
bool filesystem_change_directory(const char* path);
bool filesystem_get_current_directory(char* buffer, uint32_t buffer_size);
uint32_t filesystem_list_directory(const char* path, fs_dir_entry_t* entries, uint32_t max_entries);

bool filesystem_create_file(const char* path);
bool filesystem_delete_file(const char* path);
bool filesystem_copy_file(const char* source, const char* destination);
bool filesystem_move_file(const char* source, const char* destination);
bool filesystem_file_exists(const char* path);
bool filesystem_get_file_info(const char* path, fs_file_info_t* info);

fs_file_handle_t* filesystem_open_file(const char* path, bool write_mode);
void filesystem_close_file(fs_file_handle_t* handle);
uint32_t filesystem_read_file(fs_file_handle_t* handle, void* buffer, uint32_t size);
uint32_t filesystem_write_file(fs_file_handle_t* handle, const void* buffer, uint32_t size);
bool filesystem_seek_file(fs_file_handle_t* handle, uint32_t position);

bool filesystem_resolve_path(const char* path, char* resolved_path, uint32_t buffer_size);
uint32_t filesystem_get_free_space(void);
uint32_t filesystem_get_used_space(void);
void filesystem_format(void);

typedef struct {
    uint32_t total_files;
    uint32_t total_directories;
    uint32_t free_blocks;
    uint32_t used_blocks;
    uint32_t total_space;
    uint32_t free_space;
} fs_stats_t;

bool filesystem_get_stats(fs_stats_t* stats);

#endif