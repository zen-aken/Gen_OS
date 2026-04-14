//* Physical Memory Manager - Stack-based page allocator for 64-bit systems
//  Supports full 64-bit physical address space using a free-page stack
//  Stack stored in lowest usable memory region during bootstrap

#include "pmm.h"
#include <limine.h>
#include <log.h>

// Maximum number of pages we can track in our initial stack
// For 128MB of RAM = 32768 pages. We support up to 64GB = 16M pages comfortably.
// Stack grows downward from high addresses in a reserved region.
#define PMM_MAX_PAGES (64ULL * 1024 * 1024 * 1024 / PAGE_SIZE)  // 64GB worth of pages

// External Limine requests
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_executable_address_request executable_address_request;
extern volatile struct limine_hhdm_request hhdm_request;

// Stack-based free page tracking
// We store free page physical addresses in a stack allocated from early memory
static uint64_t *free_stack = NULL;     // Array of free page addresses
static size_t stack_capacity = 0;       // Max entries stack can hold
static size_t stack_top = 0;            // Current stack position (0 = empty)
static uint64_t stack_base_phys = 0;    // Physical address where stack lives
static size_t total_usable_pages = 0;
static size_t reserved_early_pages = 0;

/**
 * @brief Push page address onto free stack
 */
static inline bool stack_push(uint64_t addr) {
    if (stack_top >= stack_capacity) return false;
    free_stack[stack_top++] = addr;
    return true;
}

/**
 * @brief Pop page address from free stack
 */
static inline uint64_t stack_pop(void) {
    if (stack_top == 0) return 0;
    return free_stack[--stack_top];
}

/**
 * @brief Initialize the PMM with Limine memory map.
 * 
 * Strategy:
 * 1. First pass: count usable pages and find lowest usable region
 * 2. Reserve early pages for the stack itself
 * 3. Build stack by iterating usable regions, skipping reserved pages
 */
void pmm_init(void) {
    if (memmap_request.response == NULL) {
        log(LOG_TYPE_ERROR, "[ PMM ] No memory map from bootloader!\n");
        return;
    }

    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_memmap_entry **entries = memmap->entries;
    uint64_t entry_count = memmap->entry_count;

    // First pass: count total usable pages and find lowest region for stack
    total_usable_pages = 0;
    size_t pages_below_4gb = 0;
    uint64_t lowest_usable_base = 0xFFFFFFFFFFFFFFFF;
    uint64_t lowest_usable_len = 0;

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
            uint64_t page_count = entry->length >> PAGE_SHIFT;
            total_usable_pages += page_count;

            // Track pages below 4GB for early stack allocation
            if (entry->base < 0x100000000ULL) {
                uint64_t end = entry->base + entry->length;
                if (end > 0x100000000ULL) end = 0x100000000ULL;
                pages_below_4gb += (end - entry->base) >> PAGE_SHIFT;
            }

            // Find lowest usable region (preferably below 1MB for early boot simplicity)
            if (entry->base < lowest_usable_base && entry->length >= PAGE_SIZE) {
                lowest_usable_base = entry->base;
                lowest_usable_len = entry->length;
            }
        }

        log(LOG_TYPE_INFO, "[ MEMMAP ] 0x%16x - 0x%16x | %d MB | %s\n",
            entry->base,
            entry->base + entry->length - 1,
            (int)(entry->length / (1024 * 1024)),
            type_str);
    }

    if (total_usable_pages == 0) {
        log(LOG_TYPE_ERROR, "[ PMM ] No usable memory found!\n");
        return;
    }

    log(LOG_TYPE_INFO, "[ PMM ] Total usable pages: %d (%d MB)\n",
        total_usable_pages,
        (int)((total_usable_pages * PAGE_SIZE) / (1024 * 1024)));

    // Calculate stack size needed (8 bytes per entry)
    size_t stack_size_needed = total_usable_pages * sizeof(uint64_t);
    stack_size_needed = (stack_size_needed + PAGE_SIZE - 1) & PAGE_MASK;  // Round up

    // Reserve pages for stack from lowest usable memory
    // Stack must fit entirely within usable memory
    reserved_early_pages = stack_size_needed >> PAGE_SHIFT;

    if (lowest_usable_len < stack_size_needed + PAGE_SIZE) {
        log(LOG_TYPE_ERROR, "[ PMM ] Lowest usable region too small for stack!\n");
        return;
    }

    // Place stack at start of lowest usable region
    stack_base_phys = lowest_usable_base;
    // In higher-half kernel, physical addresses need offset. We assume
    // Limine maps physical 0 at 0xFFFF800000000000 or we access early via identity
    // For now, we rely on the HHDM offset that Limine provides

    // Get HHDM (Higher Half Direct Map) offset from Limine
    uint64_t hhdm_offset = 0xFFFF800000000000ULL;  // Default fallback
    if (hhdm_request.response) {
        hhdm_offset = hhdm_request.response->offset;
    }
    free_stack = (uint64_t *)(stack_base_phys + hhdm_offset);
    stack_capacity = (stack_size_needed / sizeof(uint64_t)) - 1;  // Leave margin

    log(LOG_TYPE_INFO, "[ PMM ] Stack at 0x%x (virt 0x%x), capacity %d pages\n",
        stack_base_phys, (uint64_t)free_stack, stack_capacity);

    // Initialize stack as empty
    stack_top = 0;

    // Second pass: populate stack with all free pages, except reserved ones
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t base = (entry->base + PAGE_SIZE - 1) & PAGE_MASK;  // Round up
        uint64_t end = entry->base + entry->length;
        end = end & PAGE_MASK;  // Round down

        for (uint64_t addr = base; addr < end; addr += PAGE_SIZE) {
            // Skip the stack pages themselves
            if (addr >= stack_base_phys && addr < stack_base_phys + stack_size_needed) {
                continue;
            }

            // Skip NULL page (first page) for safety
            if (addr == 0) continue;

            // Try to push to stack
            if (!stack_push(addr)) {
                log(LOG_TYPE_WARNING, "[ PMM ] Stack full, dropping page 0x%x\n", addr);
                break;
            }
        }
    }

    // Reserve kernel space
    if (executable_address_request.response) {
        uint64_t kernel_phys = executable_address_request.response->physical_base;
        uint64_t kernel_size = 0x200000;  // Assume 2MB kernel

        log(LOG_TYPE_INFO, "[ PMM ] Reserving kernel at 0x%x (2MB)\n", kernel_phys);

        // Remove kernel pages from stack (mark as used)
        for (uint64_t addr = kernel_phys; addr < kernel_phys + kernel_size; addr += PAGE_SIZE) {
            // Scan stack and remove kernel pages (inefficient but only done once)
            for (size_t j = 0; j < stack_top; j++) {
                if (free_stack[j] == addr) {
                    // Remove by replacing with top element and popping
                    free_stack[j] = free_stack[--stack_top];
                    break;
                }
            }
        }
    }

    log(LOG_TYPE_INFO, "[ PMM ] Initialized: %d pages free (%d MB)\n",
        stack_top,
        (int)((stack_top * PAGE_SIZE) / (1024 * 1024)));
}

/**
 * @brief Allocate a single physical page from the stack.
 */
uint64_t pmm_alloc_page(void) {
    if (stack_top == 0) return 0;
    return stack_pop();
}

/**
 * @brief Free a page by pushing it back onto the stack.
 */
void pmm_free_page(uint64_t addr) {
    if (addr == 0) return;
    if ((addr & (PAGE_SIZE - 1)) != 0) return;  // Must be aligned

    // Cannot free pages that are part of the stack itself
    uint64_t stack_end = stack_base_phys + (reserved_early_pages << PAGE_SHIFT);
    if (addr >= stack_base_phys && addr < stack_end) {
        log(LOG_TYPE_WARNING, "[ PMM ] Attempt to free stack page 0x%x ignored\n", addr);
        return;
    }

    stack_push(addr);
}

/**
 * @brief Allocate n contiguous pages.
 * This is expensive on a stack-based allocator - we must search.
 */
uint64_t pmm_alloc_pages(size_t n) {
    if (n == 0 || n > stack_top) return 0;

    // For contiguous allocation, we need to find n consecutive addresses
    // This is O(N^2) in worst case - acceptable for occasional use only

    // Simple approach: sort a temporary view of the stack (bubble sort for small N)
    // Actually, let's do linear scan for runs

    for (size_t i = 0; i <= stack_top - n; i++) {
        uint64_t base = free_stack[i];
        size_t found = 1;

        // Check if n consecutive pages exist starting from base
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
            // Found contiguous block, remove all pages from stack
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

    return 0;  // No contiguous block found
}

/**
 * @brief Free n contiguous pages.
 */
void pmm_free_pages(uint64_t addr, size_t n) {
    if (addr == 0 || n == 0) return;

    for (size_t i = 0; i < n; i++) {
        pmm_free_page(addr + (i << PAGE_SHIFT));
    }
}

/**
 * @brief Get count of free pages.
 */
size_t pmm_free_count(void) {
    return stack_top;
}

/**
 * @brief Get total usable physical memory (excluding reserved).
 */
uint64_t pmm_total_memory(void) {
    return (uint64_t)stack_top * PAGE_SIZE;
}
