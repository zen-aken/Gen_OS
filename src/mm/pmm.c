//* Physical Memory Manager - Stack-based page allocator for 64-bit systems
//  Supports full 64-bit physical address space using a free-page stack
//  Only uses memory above 1MB (0x100000) as low memory isn't HHDM-mapped

#include "pmm.h"
#include <limine.h>
#include <log.h>

// Minimum address to use (1MB - below this HHDM may not cover)
#define PMM_MIN_ADDRESS 0x100000ULL

// Stack-based free page tracking
static uint64_t *free_stack = NULL;
static size_t stack_capacity = 0;
static size_t stack_top = 0;
static uint64_t hhdm_offset = 0;

static inline bool stack_push(uint64_t addr) {
    if (stack_top >= stack_capacity) return false;
    free_stack[stack_top++] = addr;
    return true;
}

static inline uint64_t stack_pop(void) {
    if (stack_top == 0) return 0;
    return free_stack[--stack_top];
}

static inline uint64_t phys_to_virt(uint64_t phys) {
    return phys + hhdm_offset;
}

/**
 * @brief Initialize PMM using only memory above 1MB (which Limine HHDM maps).
 */
void pmm_init(void) {
    extern volatile struct limine_memmap_request memmap_request;
    extern volatile struct limine_executable_address_request executable_address_request;
    extern volatile struct limine_hhdm_request hhdm_request;

    if (memmap_request.response == NULL) {
        log(LOG_TYPE_ERROR, "[ PMM ] No memory map from bootloader!\n");
        return;
    }

    // Get HHDM offset
    hhdm_offset = 0xFFFF800000000000ULL;
    if (hhdm_request.response) {
        hhdm_offset = hhdm_request.response->offset;
    }
    log(LOG_TYPE_INFO, "[ PMM ] HHDM offset: 0x%x\n", hhdm_offset);

    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_memmap_entry **entries = memmap->entries;
    uint64_t entry_count = memmap->entry_count;

    // Count usable pages (only above 1MB - low memory is not HHDM-mapped)
    size_t total_usable_pages = 0;
    uint64_t best_stack_region_base = 0;
    uint64_t best_stack_region_len = 0;

    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        const char *type_str = "Unknown";
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE: type_str = "Usable"; break;
            case LIMINE_MEMMAP_RESERVED: type_str = "Reserved"; break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE: type_str = "ACPI Reclaim"; break;
            case LIMINE_MEMMAP_ACPI_NVS: type_str = "ACPI NVS"; break;
            case LIMINE_MEMMAP_BAD_MEMORY: type_str = "Bad"; break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: type_str = "Boot Reclaim"; break;
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: type_str = "Kernel"; break;
            case LIMINE_MEMMAP_FRAMEBUFFER: type_str = "Framebuffer"; break;
        }

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            // Only count pages above 1MB (HHDM-mapped)
            uint64_t usable_start = entry->base;
            uint64_t usable_end = entry->base + entry->length;

            if (usable_start < PMM_MIN_ADDRESS) {
                usable_start = PMM_MIN_ADDRESS;
            }

            if (usable_end > usable_start) {
                uint64_t usable_len = usable_end - usable_start;
                uint64_t page_count = usable_len >> PAGE_SHIFT;
                total_usable_pages += page_count;

                // Find largest usable region above 1MB for stack
                if (usable_len > best_stack_region_len) {
                    best_stack_region_base = usable_start;
                    best_stack_region_len = usable_len;
                }
            }
        }

        log(LOG_TYPE_INFO, "[ MEMMAP ] 0x%10x - 0x%10x | %3d MB | %s\n",
            entry->base,
            entry->base + entry->length - 1,
            (int)(entry->length / (1024 * 1024)),
            type_str);
    }

    if (total_usable_pages == 0) {
        log(LOG_TYPE_ERROR, "[ PMM ] No usable memory found above 1MB!\n");
        return;
    }

    log(LOG_TYPE_INFO, "[ PMM ] Total usable pages (>1MB): %d (%d MB)\n",
        total_usable_pages,
        (int)((total_usable_pages * PAGE_SIZE) / (1024 * 1024)));

    // Calculate stack size (8 bytes per page entry)
    size_t stack_size = total_usable_pages * sizeof(uint64_t);
    stack_size = (stack_size + PAGE_SIZE - 1) & PAGE_MASK;

    if (best_stack_region_len < stack_size + PAGE_SIZE) {
        log(LOG_TYPE_ERROR, "[ PMM ] No region large enough for stack!\n");
        return;
    }

    // Place stack at the start of the best region
    uint64_t stack_phys = best_stack_region_base;
    free_stack = (uint64_t *)phys_to_virt(stack_phys);
    stack_capacity = (stack_size / sizeof(uint64_t)) - 1;

    // Verify we can access the stack (touch first page)
    volatile uint64_t *test = free_stack;
    *test = 0xDEADBEEFCAFEBABEULL;
    if (*test != 0xDEADBEEFCAFEBABEULL) {
        log(LOG_TYPE_ERROR, "[ PMM ] Cannot access stack memory at 0x%x (virt 0x%x)!\n",
            stack_phys, (uint64_t)free_stack);
        return;
    }

    log(LOG_TYPE_INFO, "[ PMM ] Stack at phys 0x%x (virt 0x%x), capacity %d pages\n",
        stack_phys, (uint64_t)free_stack, stack_capacity);

    // Populate stack with free pages, skipping the stack itself
    stack_top = 0;
    uint64_t stack_end = stack_phys + stack_size;

    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t base = entry->base;
        uint64_t end = entry->base + entry->length;

        // Only use memory above 1MB
        if (base < PMM_MIN_ADDRESS) base = PMM_MIN_ADDRESS;
        if (base >= end) continue;

        // Round to page boundaries
        base = (base + PAGE_SIZE - 1) & PAGE_MASK;
        end = end & PAGE_MASK;

        for (uint64_t addr = base; addr < end; addr += PAGE_SIZE) {
            // Skip stack pages
            if (addr >= stack_phys && addr < stack_end) continue;

            if (!stack_push(addr)) {
                log(LOG_TYPE_WARNING, "[ PMM ] Stack full, dropping page 0x%x\n", addr);
                break;
            }
        }
    }

    // Remove kernel pages from stack
    if (executable_address_request.response) {
        uint64_t kernel_phys = executable_address_request.response->physical_base;
        // Kernel is typically mapped in higher half, but physical is low
        // Check if physical kernel is in our free list
        for (uint64_t i = 0; i < 0x200000; i += PAGE_SIZE) {
            uint64_t addr = kernel_phys + i;
            for (size_t j = 0; j < stack_top; j++) {
                if (free_stack[j] == addr) {
                    free_stack[j] = free_stack[--stack_top];
                    break;
                }
            }
        }
        log(LOG_TYPE_INFO, "[ PMM ] Reserved kernel pages\n");
    }

    log(LOG_TYPE_INFO, "[ PMM ] Initialized: %d pages free (%d MB)\n",
        stack_top,
        (int)((stack_top * PAGE_SIZE) / (1024 * 1024)));
}

uint64_t pmm_alloc_page(void) {
    if (stack_top == 0) return 0;
    return stack_pop();
}

void pmm_free_page(uint64_t addr) {
    if (addr == 0) return;
    if ((addr & (PAGE_SIZE - 1)) != 0) return;
    if (addr < PMM_MIN_ADDRESS) return;  // Can't free low memory
    stack_push(addr);
}

uint64_t pmm_alloc_pages(size_t n) {
    if (n == 0 || n > stack_top) return 0;

    // Simple search for contiguous block
    for (size_t i = 0; i <= stack_top - n; i++) {
        uint64_t base = free_stack[i];
        size_t found = 1;

        for (size_t j = 1; j < n && i + j < stack_top; j++) {
            bool found_next = false;
            for (size_t k = i + j; k < stack_top; k++) {
                if (free_stack[k] == base + (j << PAGE_SHIFT)) {
                    found_next = true;
                    break;
                }
            }
            if (!found_next) break;
            found++;
        }

        if (found == n) {
            for (size_t j = 0; j < n; j++) {
                uint64_t target = base + (j << PAGE_SHIFT);
                for (size_t k = i; k < stack_top; k++) {
                    if (free_stack[k] == target) {
                        free_stack[k] = free_stack[--stack_top];
                        break;
                    }
                }
            }
            return base;
        }
    }
    return 0;
}

void pmm_free_pages(uint64_t addr, size_t n) {
    if (addr == 0 || n == 0) return;
    for (size_t i = 0; i < n; i++) {
        pmm_free_page(addr + (i << PAGE_SHIFT));
    }
}

size_t pmm_free_count(void) {
    return stack_top;
}

uint64_t pmm_total_memory(void) {
    return (uint64_t)stack_top * PAGE_SIZE;
}
