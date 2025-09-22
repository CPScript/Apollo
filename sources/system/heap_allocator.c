#include "heap_allocator.h"
#include <stdint.h>
#include <stdbool.h>

#define HEAP_SIZE_BYTES (16 * 1024 * 1024)  // 16MB heap
#define ALIGNMENT_SIZE 8
#define MIN_BLOCK_SIZE (sizeof(memory_block_t))

typedef struct memory_block {
    size_t block_size;
    bool is_allocated;
    struct memory_block* next_block;
    struct memory_block* previous_block;
} memory_block_t;

typedef struct {
    uint8_t* heap_base;
    size_t total_size;
    memory_block_t* first_block;
    bool is_initialized;
} heap_manager_t;

static uint8_t heap_storage[HEAP_SIZE_BYTES] __attribute__((aligned(4096)));
static heap_manager_t heap_manager = {0};

static size_t align_size(size_t size) {
    return (size + ALIGNMENT_SIZE - 1) & ~(ALIGNMENT_SIZE - 1);
}

static memory_block_t* get_block_from_ptr(void* ptr) {
    if (!ptr) return NULL;
    return (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));
}

static void* get_ptr_from_block(memory_block_t* block) {
    if (!block) return NULL;
    return (uint8_t*)block + sizeof(memory_block_t);
}

static memory_block_t* find_suitable_block(size_t required_size) {
    memory_block_t* current = heap_manager.first_block;
    
    while (current) {
        if (!current->is_allocated && current->block_size >= required_size) {
            return current;
        }
        current = current->next_block;
    }
    
    return NULL; 
}

static void split_block_if_needed(memory_block_t* block, size_t requested_size) {
    size_t remaining_size = block->block_size - requested_size - sizeof(memory_block_t);
    
    if (remaining_size >= MIN_BLOCK_SIZE) {
        memory_block_t* new_block = (memory_block_t*)((uint8_t*)block + sizeof(memory_block_t) + requested_size);
        
        new_block->block_size = remaining_size;
        new_block->is_allocated = false;
        new_block->next_block = block->next_block;
        new_block->previous_block = block;
        
        if (block->next_block) {
            block->next_block->previous_block = new_block;
        }
        
        block->next_block = new_block;
        block->block_size = requested_size;
    }
}

static void merge_free_blocks(memory_block_t* block) {
    if (!block || block->is_allocated) return;
    
    while (block->next_block && !block->next_block->is_allocated) {
        memory_block_t* next = block->next_block;
        block->block_size += sizeof(memory_block_t) + next->block_size;
        block->next_block = next->next_block;
        
        if (next->next_block) {
            next->next_block->previous_block = block;
        }
    }
    
    if (block->previous_block && !block->previous_block->is_allocated) {
        memory_block_t* prev = block->previous_block;
        prev->block_size += sizeof(memory_block_t) + block->block_size;
        prev->next_block = block->next_block;
        
        if (block->next_block) {
            block->next_block->previous_block = prev;
        }
    }
}

void heap_allocator_initialize(void) {
    if (heap_manager.is_initialized) return;
    
    heap_manager.heap_base = heap_storage;
    heap_manager.total_size = HEAP_SIZE_BYTES;
    heap_manager.first_block = (memory_block_t*)heap_storage;
    
    heap_manager.first_block->block_size = HEAP_SIZE_BYTES - sizeof(memory_block_t);
    heap_manager.first_block->is_allocated = false;
    heap_manager.first_block->next_block = NULL;
    heap_manager.first_block->previous_block = NULL;
    
    heap_manager.is_initialized = true;
}

void* apollo_allocate_memory(size_t size) {
    if (!heap_manager.is_initialized) {
        heap_allocator_initialize();
    }
    
    if (size == 0) return NULL;
    
    size_t aligned_size = align_size(size);
    
    memory_block_t* block = find_suitable_block(aligned_size);
    if (!block) {
        return NULL; 
    }
    
    split_block_if_needed(block, aligned_size);
    
    block->is_allocated = true;
    
    return get_ptr_from_block(block);
}

void apollo_free_memory(void* ptr) {
    if (!ptr) return;
    
    memory_block_t* block = get_block_from_ptr(ptr);
    if (!block || !block->is_allocated) {
        return; 
    }
    
    block->is_allocated = false;
    
    merge_free_blocks(block);
}

void* apollo_allocate_zeroed_memory(size_t count, size_t element_size) {
    size_t total_size = count * element_size;
    void* ptr = apollo_allocate_memory(total_size);
    
    if (ptr) {
        uint8_t* byte_ptr = (uint8_t*)ptr;
        for (size_t i = 0; i < total_size; i++) {
            byte_ptr[i] = 0;
        }
    }
    
    return ptr;
}

void* apollo_reallocate_memory(void* ptr, size_t new_size) {
    if (!ptr) {
        return apollo_allocate_memory(new_size);
    }
    
    if (new_size == 0) {
        apollo_free_memory(ptr);
        return NULL;
    }
    
    memory_block_t* block = get_block_from_ptr(ptr);
    if (!block || !block->is_allocated) {
        return NULL; // Invalid pointer
    }
    
    size_t aligned_new_size = align_size(new_size);
    
    if (block->block_size >= aligned_new_size) {
        split_block_if_needed(block, aligned_new_size);
        return ptr;
    }
    
    void* new_ptr = apollo_allocate_memory(new_size);
    if (!new_ptr) {
        return NULL;
    }
    
    uint8_t* old_data = (uint8_t*)ptr;
    uint8_t* new_data = (uint8_t*)new_ptr;
    size_t copy_size = (block->block_size < new_size) ? block->block_size : new_size;
    
    for (size_t i = 0; i < copy_size; i++) {
        new_data[i] = old_data[i];
    }
    
    apollo_free_memory(ptr);
    
    return new_ptr;
}

size_t heap_allocator_get_used_memory(void) {
    if (!heap_manager.is_initialized) return 0;
    
    size_t used_memory = 0;
    memory_block_t* current = heap_manager.first_block;
    
    while (current) {
        if (current->is_allocated) {
            used_memory += current->block_size + sizeof(memory_block_t);
        }
        current = current->next_block;
    }
    
    return used_memory;
}

size_t heap_allocator_get_free_memory(void) {
    if (!heap_manager.is_initialized) return 0;
    
    size_t free_memory = 0;
    memory_block_t* current = heap_manager.first_block;
    
    while (current) {
        if (!current->is_allocated) {
            free_memory += current->block_size;
        }
        current = current->next_block;
    }
    
    return free_memory;
}

size_t heap_allocator_get_total_memory(void) {
    return HEAP_SIZE_BYTES;
}

void heap_allocator_dump_info(void) {
    // Needs to provide detailed heap information
    // For now, this is used internally for debugging
}