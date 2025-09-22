#include "process_manager.h"
#include "heap_allocator.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_PROCESSES 64
#define SCHEDULER_TIME_SLICE 10

typedef struct {
    process_t processes[MAX_PROCESSES];
    uint32_t next_pid;
    uint32_t current_pid;
    uint32_t total_context_switches;
    uint32_t system_uptime;
    bool is_initialized;
} process_manager_state_t;

static process_manager_state_t pm_state = {0};

static void string_copy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

static uint32_t string_length(const char* str) {
    uint32_t len = 0;
    while (str && str[len] != '\0') len++;
    return len;
}

static uint32_t get_system_time(void) {
    return ++pm_state.system_uptime;
}

static uint32_t find_free_process_slot(void) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (!pm_state.processes[i].is_active) {
            return i;
        }
    }
    return MAX_PROCESSES; // No free slot
}

static process_t* find_process_by_pid(uint32_t pid) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (pm_state.processes[i].is_active && pm_state.processes[i].pid == pid) {
            return &pm_state.processes[i];
        }
    }
    return NULL;
}

static void create_system_processes(void) {
    uint32_t kernel_slot = find_free_process_slot();
    if (kernel_slot < MAX_PROCESSES) {
        process_t* kernel = &pm_state.processes[kernel_slot];
        kernel->pid = 0;
        string_copy(kernel->name, "kernel");
        kernel->state = PROCESS_STATE_RUNNING;
        kernel->type = PROCESS_TYPE_KERNEL;
        kernel->priority = 255; // Highest priority
        kernel->cpu_time = 0;
        kernel->memory_usage = 2 * 1024 * 1024; // 2MB
        kernel->parent_pid = 0; // Kernel has no parent
        kernel->start_time = get_system_time();
        kernel->entry_point = NULL;
        kernel->is_active = true;
    }
    
    process_create("memory_manager", PROCESS_TYPE_SYSTEM, (void*)0x100000);
    
    process_create("vga_driver", PROCESS_TYPE_SYSTEM, (void*)0x100100);
    
    process_create("keyboard_driver", PROCESS_TYPE_SYSTEM, (void*)0x100200);
    
    process_create("filesystem", PROCESS_TYPE_SYSTEM, (void*)0x100300);
    
    process_create("shell", PROCESS_TYPE_SYSTEM, (void*)0x100400);
    
    uint32_t editor_pid = process_create("text_editor", PROCESS_TYPE_USER, (void*)0x100500);
    process_suspend(editor_pid);
    
    process_create("rtc_driver", PROCESS_TYPE_SYSTEM, (void*)0x100600);
    
    process_create("scheduler", PROCESS_TYPE_KERNEL, (void*)0x100700);
    
    pm_state.current_pid = 0; // Start with kernel process
}

void process_manager_initialize(void) {
    if (pm_state.is_initialized) return;
    
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        pm_state.processes[i].is_active = false;
    }
    
    pm_state.next_pid = 1;
    pm_state.current_pid = 0;
    pm_state.total_context_switches = 0;
    pm_state.system_uptime = 0;
    pm_state.is_initialized = true;
    
    create_system_processes();
}

uint32_t process_create(const char* name, process_type_t type, void* entry_point) {
    if (!name || string_length(name) == 0) return 0;
    
    uint32_t slot = find_free_process_slot();
    if (slot >= MAX_PROCESSES) return 0; // No free slots
    
    process_t* proc = &pm_state.processes[slot];
    
    proc->pid = pm_state.next_pid++;
    string_copy(proc->name, name);
    proc->state = PROCESS_STATE_READY;
    proc->type = type;
    
    switch (type) {
        case PROCESS_TYPE_KERNEL:
            proc->priority = 200 + (proc->pid % 55); // 200-255
            break;
        case PROCESS_TYPE_SYSTEM:
            proc->priority = 100 + (proc->pid % 99); // 100-199
            break;
        case PROCESS_TYPE_USER:
            proc->priority = 1 + (proc->pid % 99);   // 1-99
            break;
    }
    
    proc->cpu_time = 0;
    proc->memory_usage = 64 * 1024; // Default 64KB
    proc->parent_pid = pm_state.current_pid;
    proc->start_time = get_system_time();
    proc->entry_point = entry_point;
    proc->is_active = true;
    
    return proc->pid;
}

bool process_terminate(uint32_t pid) {
    if (pid == 0) return false; // Cannot terminate kernel
    
    process_t* proc = find_process_by_pid(pid);
    if (!proc) return false;
    
    proc->state = PROCESS_STATE_TERMINATED;
    proc->is_active = false;
    
    return true;
}

bool process_suspend(uint32_t pid) {
    process_t* proc = find_process_by_pid(pid);
    if (!proc) return false;
    
    if (proc->state == PROCESS_STATE_RUNNING || proc->state == PROCESS_STATE_READY) {
        proc->state = PROCESS_STATE_BLOCKED;
        return true;
    }
    
    return false;
}

bool process_resume(uint32_t pid) {
    process_t* proc = find_process_by_pid(pid);
    if (!proc) return false;
    
    if (proc->state == PROCESS_STATE_BLOCKED) {
        proc->state = PROCESS_STATE_READY;
        return true;
    }
    
    return false;
}

bool process_get_info(uint32_t pid, process_t* info) {
    if (!info) return false;
    
    process_t* proc = find_process_by_pid(pid);
    if (!proc) return false;
    
    *info = *proc;
    return true;
}

uint32_t process_list(process_t* processes, uint32_t max_count) {
    if (!processes || max_count == 0) return 0;
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < MAX_PROCESSES && count < max_count; i++) {
        if (pm_state.processes[i].is_active) {
            processes[count] = pm_state.processes[i];
            count++;
        }
    }
    
    return count;
}

bool process_get_stats(process_stats_t* stats) {
    if (!stats) return false;
    
    stats->total_processes = 0;
    stats->running_processes = 0;
    stats->blocked_processes = 0;
    stats->total_cpu_time = 0;
    
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (pm_state.processes[i].is_active) {
            stats->total_processes++;
            stats->total_cpu_time += pm_state.processes[i].cpu_time;
            
            switch (pm_state.processes[i].state) {
                case PROCESS_STATE_RUNNING:
                case PROCESS_STATE_READY:
                    stats->running_processes++;
                    break;
                case PROCESS_STATE_BLOCKED:
                    stats->blocked_processes++;
                    break;
                default:
                    break;
            }
        }
    }
    
    stats->context_switches = pm_state.total_context_switches;
    
    return true;
}

uint32_t process_get_current_pid(void) {
    return pm_state.current_pid;
}

void process_yield(void) {
    uint32_t start_search = (pm_state.current_pid + 1) % MAX_PROCESSES;
    uint32_t search_pos = start_search;
    
    do {
        if (pm_state.processes[search_pos].is_active && 
            pm_state.processes[search_pos].state == PROCESS_STATE_READY) {
            
            process_t* current = find_process_by_pid(pm_state.current_pid);
            if (current && current->state == PROCESS_STATE_RUNNING) {
                current->state = PROCESS_STATE_READY;
            }
            
            pm_state.current_pid = pm_state.processes[search_pos].pid;
            pm_state.processes[search_pos].state = PROCESS_STATE_RUNNING;
            pm_state.total_context_switches++;
            
            return;
        }
        
        search_pos = (search_pos + 1) % MAX_PROCESSES;
    } while (search_pos != start_search);
    
}

void process_scheduler_tick(void) {
    process_t* current = find_process_by_pid(pm_state.current_pid);
    if (current) {
        current->cpu_time++;
        
        if (current->type == PROCESS_TYPE_USER && 
            (current->cpu_time % SCHEDULER_TIME_SLICE) == 0) {
            process_yield();
        }
    }
    
    pm_state.system_uptime++;
}

void process_update_memory_usage(uint32_t pid, uint32_t memory_bytes) {
    process_t* proc = find_process_by_pid(pid);
    if (proc) {
        proc->memory_usage = memory_bytes;
    }
}

void process_update_cpu_time(uint32_t pid, uint32_t time_slice) {
    process_t* proc = find_process_by_pid(pid);
    if (proc) {
        proc->cpu_time += time_slice;
    }
}

// Simulate process activity based on system operations
void process_simulate_activity(void) {
    uint32_t heap_used = heap_allocator_get_used_memory();
    process_update_memory_usage(1, heap_used / 4); // Memory manager
    
    process_update_cpu_time(2, 1);
    
    extern bool input_manager_has_input(void);
    if (input_manager_has_input()) {
        process_update_cpu_time(3, 2);
    }
    
    process_update_cpu_time(4, 1);
    
    process_update_cpu_time(5, 3);
    
    process_update_cpu_time(8, 1);
}

const char* process_get_name(uint32_t pid) {
    process_t* proc = find_process_by_pid(pid);
    if (proc) {
        return proc->name;
    }
    return "unknown";
}

const char* process_get_state_string(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_RUNNING: return "running";
        case PROCESS_STATE_READY: return "ready";
        case PROCESS_STATE_BLOCKED: return "blocked";
        case PROCESS_STATE_TERMINATED: return "terminated";
        default: return "unknown";
    }
}

const char* process_get_type_string(process_type_t type) {
    switch (type) {
        case PROCESS_TYPE_KERNEL: return "kernel";
        case PROCESS_TYPE_SYSTEM: return "system";
        case PROCESS_TYPE_USER: return "user";
        default: return "unknown";
    }
}