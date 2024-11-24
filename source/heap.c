#include "heap.h"
#include <nds/debug.h>
#include <stdio.h>
#include <string.h>

typedef struct BlockHeader {
    u32 size;
    struct BlockHeader* next;
    int free;
} BlockHeader;

static uint8_t* heap_start;
static u32 heap_size;
static BlockHeader* free_list;

void heap_init(void* start, u32 size) {
    heap_start = (uint8_t*)start;
    heap_size = size;

    free_list = (BlockHeader*)heap_start;
    free_list->size = heap_size - sizeof(BlockHeader);
    free_list->next = NULL;
    free_list->free = 1;
}

void* malloc(u32 size) {
    BlockHeader* current = free_list;
    BlockHeader* previous = NULL;

    while (current != NULL) {
        if (current->free && current->size >= size) {
            if (current->size > size + sizeof(BlockHeader)) {
                BlockHeader* new_block = (BlockHeader*)((uint8_t*)current + sizeof(BlockHeader) + size);
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->next = current->next;
                new_block->free = 1;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;
            return (void*)((uint8_t*)current + sizeof(BlockHeader));
        }

        previous = current;
        current = current->next;
    }

    return NULL; // No suitable block found
}

void free(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    block->free = 1;

    // Coalesce adjacent free blocks
    BlockHeader* current = free_list;
    while (current != NULL) {
        if (current->free && current->next != NULL && current->next->free) {
            current->size += current->next->size + sizeof(BlockHeader);
            current->next = current->next->next;
        }
        current = current->next;
    }
}

void* calloc(u32 nmemb, u32 size) {
    u32 total_size = nmemb * size;
    void* ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* realloc(void* ptr, u32 size) {
    if (ptr == NULL) {
        return malloc(size);
    }

    BlockHeader* block = (BlockHeader*)((uint8_t*)ptr - sizeof(BlockHeader));
    if (block->size >= size) {
        return ptr;
    }

    void* new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        free(ptr);
    }
    return new_ptr;
}