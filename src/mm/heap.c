//* Kernel Heap Allocator - Simple segregated fit with page backing
//  Allocates virtual pages from VMM, splits into blocks

#include "heap.h"
#include "vmm.h"
#include "pmm.h"
#include "utils/string.h"
#include <log.h>

#define HEAP_MAGIC 0xDEADCAFEBABEBEEFULL
#define HEAP_PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 16
#define HEAP_PAGES_PER_CHUNK 16  // 64KB per chunk

// Block header
struct heap_block {
    uint64_t magic;
    size_t size;           // Total block size (including header)
    bool used;
    struct heap_block *next;
    struct heap_block *prev;
};

#define HEADER_SIZE sizeof(struct heap_block)
#define BLOCK_DATA(b) ((void *)((uint64_t)(b) + HEADER_SIZE))
#define DATA_TO_BLOCK(p) ((struct heap_block *)((uint64_t)(p) - HEADER_SIZE))

// Free lists by size class (powers of 2)
#define NUM_SIZE_CLASSES 12  // 16, 32, 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, >16K

static struct heap_block *free_lists[NUM_SIZE_CLASSES] = {0};
static uint64_t heap_current_vaddr = HEAP_START_VADDR;
static size_t heap_total = 0;
static size_t heap_used = 0;

static inline int size_to_class(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    if (size <= 2048) return 7;
    if (size <= 4096) return 8;
    if (size <= 8192) return 9;
    if (size <= 16384) return 10;
    return 11;
}

static inline size_t align_up(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

static bool expand_heap(size_t need_bytes) {
    size_t pages_needed = (need_bytes + HEAP_PAGE_SIZE - 1) / HEAP_PAGE_SIZE;
    pages_needed = (pages_needed + HEAP_PAGES_PER_CHUNK - 1) & ~(HEAP_PAGES_PER_CHUNK - 1);

    for (size_t i = 0; i < pages_needed; i++) {
        if (!vmm_alloc(heap_current_vaddr + (i * HEAP_PAGE_SIZE), VMM_KERNEL_FLAGS)) {
            log(LOG_TYPE_ERROR, "[ HEAP ] Failed to expand heap at 0x%x\n",
                heap_current_vaddr + (i * HEAP_PAGE_SIZE));
            return false;
        }
    }

    // Create one big free block from new pages
    struct heap_block *block = (struct heap_block *)heap_current_vaddr;
    block->magic = HEAP_MAGIC;
    block->size = pages_needed * HEAP_PAGE_SIZE;
    block->used = false;
    block->next = free_lists[NUM_SIZE_CLASSES - 1];
    block->prev = NULL;
    if (free_lists[NUM_SIZE_CLASSES - 1]) {
        free_lists[NUM_SIZE_CLASSES - 1]->prev = block;
    }
    free_lists[NUM_SIZE_CLASSES - 1] = block;

    heap_current_vaddr += pages_needed * HEAP_PAGE_SIZE;
    heap_total += pages_needed * HEAP_PAGE_SIZE;

    return true;
}

void heap_init(void) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        free_lists[i] = NULL;
    }

    // Initial heap expansion
    if (!expand_heap(HEAP_PAGES_PER_CHUNK * HEAP_PAGE_SIZE)) {
        log(LOG_TYPE_ERROR, "[ HEAP ] Initial heap expansion failed!\n");
        return;
    }

    log(LOG_TYPE_INFO, "[ HEAP ] Initialized at 0x%x, total %d KB\n",
        HEAP_START_VADDR, heap_total / 1024);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    size_t alloc_size = align_up(size + HEADER_SIZE, MIN_BLOCK_SIZE);
    int sc = size_to_class(alloc_size);

    // Find block in free lists
    struct heap_block *block = NULL;
    for (int i = sc; i < NUM_SIZE_CLASSES; i++) {
        if (free_lists[i]) {
            block = free_lists[i];
            free_lists[i] = block->next;
            if (free_lists[i]) {
                free_lists[i]->prev = NULL;
            }
            break;
        }
    }

    // No block found, expand heap
    if (!block) {
        if (!expand_heap(alloc_size)) {
            return NULL;
        }
        // Try again - new block is in largest class
        block = free_lists[NUM_SIZE_CLASSES - 1];
        if (block) {
            free_lists[NUM_SIZE_CLASSES - 1] = block->next;
            if (free_lists[NUM_SIZE_CLASSES - 1]) {
                free_lists[NUM_SIZE_CLASSES - 1]->prev = NULL;
            }
        }
    }

    if (!block) return NULL;

    // Split block if significantly larger
    if (block->size >= alloc_size + HEADER_SIZE + MIN_BLOCK_SIZE) {
        struct heap_block *new_block = (struct heap_block *)((uint64_t)block + alloc_size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - alloc_size;
        new_block->used = false;

        int new_sc = size_to_class(new_block->size);
        new_block->next = free_lists[new_sc];
        new_block->prev = NULL;
        if (free_lists[new_sc]) {
            free_lists[new_sc]->prev = new_block;
        }
        free_lists[new_sc] = new_block;

        block->size = alloc_size;
    }

    block->used = true;
    block->next = NULL;
    block->prev = NULL;
    heap_used += block->size;

    return BLOCK_DATA(block);
}

void *kmalloc_aligned(size_t size, size_t align) {
    if (align < 8) align = 8;

    // Allocate extra to allow for alignment adjustment
    void *raw = kmalloc(size + align + HEADER_SIZE);
    if (!raw) return NULL;

    uint64_t addr = (uint64_t)raw;
    uint64_t aligned = (addr + align - 1) & ~(align - 1);

    if (addr == aligned) return raw;

    // Store adjustment before aligned address
    uint8_t *adj = (uint8_t *)(aligned - 1);
    *adj = (uint8_t)(aligned - addr);

    return (void *)aligned;
}

void *kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) {
        uint8_t *b = p;
        for (size_t i = 0; i < size; i++) {
            b[i] = 0;
        }
    }
    return p;
}

void kfree(void *ptr) {
    if (!ptr) return;

    // Handle aligned allocations
    uint64_t addr = (uint64_t)ptr;
    if (addr & 0xF) {
        // Likely aligned, check adjustment
        uint8_t adj = *((uint8_t *)(addr - 1));
        if (adj > 0 && adj < 64) {
            addr -= adj;
        }
    }

    struct heap_block *block = DATA_TO_BLOCK((void *)addr);
    if (block->magic != HEAP_MAGIC) {
        log(LOG_TYPE_ERROR, "[ HEAP ] kfree: invalid magic at 0x%x\n", ptr);
        return;
    }
    if (!block->used) {
        log(LOG_TYPE_WARNING, "[ HEAP ] kfree: double free at 0x%x\n", ptr);
        return;
    }

    block->used = false;
    heap_used -= block->size;

    // Coalesce with next if free
    struct heap_block *next = (struct heap_block *)((uint64_t)block + block->size);
    if ((uint64_t)next < heap_current_vaddr && !next->used) {
        // Remove next from its free list
        if (next->prev) {
            next->prev->next = next->next;
        } else {
            int next_sc = size_to_class(next->size);
            if (free_lists[next_sc] == next) {
                free_lists[next_sc] = next->next;
            }
        }
        if (next->next) {
            next->next->prev = next->prev;
        }

        block->size += next->size;
    }

    // Add to free list
    int sc = size_to_class(block->size);
    block->next = free_lists[sc];
    block->prev = NULL;
    if (free_lists[sc]) {
        free_lists[sc]->prev = block;
    }
    free_lists[sc] = block;
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    struct heap_block *block = DATA_TO_BLOCK(ptr);
    if (block->magic != HEAP_MAGIC) {
        return kmalloc(new_size);  // Invalid, but try fresh alloc
    }

    size_t old_data_size = block->size - HEADER_SIZE;
    if (new_size <= old_data_size) return ptr;  // Fits in current block

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;

    // Copy old data
    uint8_t *src = ptr;
    uint8_t *dst = new_ptr;
    for (size_t i = 0; i < old_data_size; i++) {
        dst[i] = src[i];
    }

    kfree(ptr);
    return new_ptr;
}

size_t heap_total_size(void) {
    return heap_total;
}

size_t heap_free_space(void) {
    return heap_total - heap_used;
}
