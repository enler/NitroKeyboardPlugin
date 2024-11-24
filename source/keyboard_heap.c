#include <nds/ndstypes.h>
#include <string.h>

typedef struct BlockHeader {
    u32 size;
    struct BlockHeader* next;
    int free;
} BlockHeader;

static u8* heap_start;
static u32 heap_size;
static BlockHeader* free_list;

#define ALIGN_SIZE 4
#define ALIGN(x) (((x) + (ALIGN_SIZE - 1)) & ~(ALIGN_SIZE - 1))
#define MIN_BLOCK_SIZE (sizeof(BlockHeader) + ALIGN_SIZE)

void InitHeap(void *start, u32 size) {
    if (!start || size < MIN_BLOCK_SIZE) return;

    u32 addr = (u32)start;
    if (addr & (ALIGN_SIZE - 1)) {
        u32 adjust = ALIGN_SIZE - (addr & (ALIGN_SIZE - 1));
        start = (void*)((u8*)start + adjust);
        size -= adjust;
    }
    
    heap_start = (u8*)start;
    heap_size = size & ~(ALIGN_SIZE - 1);

    free_list = (BlockHeader*)heap_start;
    free_list->size = heap_size - sizeof(BlockHeader);
    free_list->next = NULL;
    free_list->free = 1;
}

void *malloc(u32 size) {
    if (size == 0) 
        return NULL;

    size = ALIGN(size);
    
    BlockHeader* current = free_list;
    BlockHeader* previous = NULL;

    while (current != NULL) {
        if (current->free && current->size >= size) {
            if (current->size >= size + MIN_BLOCK_SIZE) {
                BlockHeader* new_block = (BlockHeader*)((u8*)current + sizeof(BlockHeader) + size);
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->next = current->next;
                new_block->free = 1;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;
            return (void*)((u8*)current + sizeof(BlockHeader));
        }

        previous = current;
        current = current->next;
    }

    return NULL;
}

void free(void* ptr) {
    if (ptr == NULL) return;
    
    BlockHeader* block = (BlockHeader*)((u8*)ptr - sizeof(BlockHeader));
    if ((u8*)block < heap_start || 
        (u8*)block >= heap_start + heap_size) {
        return;
    }
    
    block->free = 1;

    BlockHeader* current = free_list;
    while (current != NULL && current->next != NULL) {
        if (current->free) {
            while (current->next != NULL && current->next->free) {
                current->size += current->next->size + sizeof(BlockHeader);
                current->next = current->next->next;
            }
        }
        current = current->next;
    }
}

void *calloc(u32 nmemb, u32 size) {
    if (nmemb != 0 && size > UINT32_MAX / nmemb) {
        return NULL;
    }
    
    u32 total_size = nmemb * size;
    void* ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void *realloc(void *ptr, u32 size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    BlockHeader* block = (BlockHeader*)((u8*)ptr - sizeof(BlockHeader));

    if ((u8*)block < heap_start || 
        (u8*)block >= heap_start + heap_size) {
        return NULL;
    }

    size = ALIGN(size);


    if (block->size >= size) {
        if (block->size >= size + MIN_BLOCK_SIZE) {
            BlockHeader* new_block = (BlockHeader*)((u8*)ptr + size);
            new_block->size = block->size - size - sizeof(BlockHeader);
            new_block->next = block->next;
            new_block->free = 1;
            block->size = size;
            block->next = new_block;
        }
        return ptr;
    }

    void* new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, block->size);
        free(ptr);
    }
    return new_ptr;
}
