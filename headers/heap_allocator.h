#ifndef APOLLO_HEAP_ALLOCATOR_H
#define APOLLO_HEAP_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

void heap_allocator_initialize(void);

void* apollo_allocate_memory(size_t size);
void apollo_free_memory(void* ptr);
void* apollo_allocate_zeroed_memory(size_t count, size_t element_size);
void* apollo_reallocate_memory(void* ptr, size_t new_size);

size_t heap_allocator_get_used_memory(void);
size_t heap_allocator_get_free_memory(void);
size_t heap_allocator_get_total_memory(void);

void heap_allocator_dump_info(void);

#endif