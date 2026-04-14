//* Physical Memory Manager - Bitmap-based allocator
//  Uses Limine memory map to track free/used pages

#include "pmm.h"
#include <limine.h>
#include <log.h>
#include <string.h>  // for memset

#define BITMAP_ELEMENT_SIZE 64
#define MAX_PHYS_PAGES (4ULL * 1024 * 1024 * 1024 / PAGE_SIZE)  // Support up to 4GB
#define BITMAP_ARRAY_SIZE ((MAX_PHYS_PAGES + BITMAP_ELEMENT_SIZE - 1) / BITMAP_ELEMENT_SIZE)

// Static bitmap - reserve space for 4GB of memory (128KB bitmap)
// Placed in BSS section, will be properly initialized during pmm_init
static uint64_t bitmap[BITMAP_ARRAY_SIZE];
static size_t bitmap_initialized = 0;

static size_t total_pages = 0;
static size_t free_pages = 0;
static uint64_t phys_base = 0;

static inline void bitmap_set(size_t page) {
    if (page / BITMAP_ELEMENT_SIZE < BITMAP_ARRAY_SIZE)
        bitmap[page / BITMAP_ELEMENT_SIZE] |= (1ULL << (page % BITMAP_ELEMENT_SIZE));
}

static inline void bitmap_clear(size_t page) {
    if (page / BITMAP_ELEMENT_SIZE < BITMAP_ARRAY_SIZE)
        bitmap[page / BITMAP_ELEMENT_SIZE] &= ~(1ULL << (page % BITMAP_ELEMENT_SIZE));
}

static inline bool bitmap_test(size_t page) {
    if (page / BITMAP_ELEMENT_SIZE >= BITMAP_ARRAY_SIZE)
        return true;  // Out of range = treated as used
    return (bitmap[page / BITMAP_ELEMENT_SIZE] >> (page % BITMAP_ELEMENT_SIZE)) & 1;
}

static inline size_t addr_to_page(uint64_t addr) {
    if (addr < phys_base) return 0;
    return (addr - phys_base) >> PAGE_SHIFT;
}

static inline uint64_t page_to_addr(size_t page) {
    return phys_base + ((uint64_t)page << PAGE_SHIFT);
}

/**
 * @brief Mark a range of pages as reserved (used).
 */
static void pmm_reserve_range(uint64_t base, uint64_t length) {
    if (length == 0 || base < phys_base) return;
    if (base >= phys_base + (MAX_PHYS_PAGES << PAGE_SHIFT)) return;

    size_t start_page = addr_to_page(base);
    size_t end_page = addr_to_page(base + length - 1) + 1;
    if (end_page > MAX_PHYS_PAGES) end_page = MAX_PHYS_PAGES;

    for (size_t i = start_page; i < end_page; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            if (free_pages > 0) free_pages--;
        }
    }
}

/**
 * @brief Mark a range of pages as free.
 */
static void pmm_free_range(uint64_t base, uint64_t length) {
    if (length == 0 || base < phys_base) return;
    if (base >= phys_base + (MAX_PHYS_PAGES << PAGE_SHIFT)) return;

    size_t start_page = addr_to_page(base);
    size_t end_page = addr_to_page(base + length - 1) + 1;
    if (end_page > MAX_PHYS_PAGES) end_page = MAX_PHYS_PAGES;

    for (size_t i = start_page; i < end_page; i++) {
        if (bitmap_test(i)) {
            bitmap_clear(i);
            free_pages++;
        }
    }
}

/**
 * @brief Initialize the PMM with Limine memory map.
 */
void pmm_init(void) {
    extern volatile struct limine_memmap_request memmap_request;
    extern volatile struct limine_executable_address_request executable_address_request;

    if (memmap_request.response == NULL) {
        log(LOG_TYPE_ERROR, "[ PMM ] No memory map response from bootloader!\n");
        return;
    }

    struct limine_memmap_response *memmap = memmap_request.response;
    struct limine_memmap_entry **entries = memmap->entries;
    uint64_t entry_count = memmap->entry_count;

    // Initialize bitmap: mark all as used first
    for (size_t i = 0; i < BITMAP_ARRAY_SIZE; i++) {
        bitmap[i] = 0xFFFFFFFFFFFFFFFFULL;
    }

    // Find lowest usable address as base
    phys_base = 0xFFFFFFFFFFFFFFFF;
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            if (entry->base < phys_base) {
                phys_base = entry->base;
            }
        }
    }

    if (phys_base == 0xFFFFFFFFFFFFFFFF) {
        log(LOG_TYPE_ERROR, "[ PMM ] No usable memory found!\n");
        return;
    }

    // Round base down to page boundary
    phys_base &= ~(PAGE_SIZE - 1);

    log(LOG_TYPE_INFO, "[ PMM ] Physical base: 0x%x\n", phys_base);

    // Mark usable regions as free
    free_pages = 0;
    total_pages = 0;
    uint64_t max_addr = 0;

    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        // Log memory map entry for debugging
        const char *type_str = "Unknown";
        switch (entry->type) {
            case LIMINE_MEMMAP_USABLE: type_str = "Usable"; break;
            case LIMINE_MEMMAP_RESERVED: type_str = "Reserved"; break;
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE: type_str = "ACPI Reclaimable"; break;
            case LIMINE_MEMMAP_ACPI_NVS: type_str = "ACPI NVS"; break;
            case LIMINE_MEMMAP_BAD_MEMORY: type_str = "Bad Memory"; break;
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE: type_str = "Bootloader Reclaimable"; break;
            case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES: type_str = "Kernel/Modules"; break;
            case LIMINE_MEMMAP_FRAMEBUFFER: type_str = "Framebuffer"; break;
        }
        log(LOG_TYPE_INFO, "[ MEMMAP ] 0x%x - 0x%x (%s)\n",
            entry->base, entry->base + entry->length - 1, type_str);

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            pmm_free_range(entry->base, entry->length);
            total_pages += (entry->length + PAGE_SIZE - 1) >> PAGE_SHIFT;
        }

        uint64_t end = entry->base + entry->length;
        if (end > max_addr) max_addr = end;
    }

    // Reserve kernel space
    if (executable_address_request.response) {
        uint64_t kernel_phys = executable_address_request.response->physical_base;
        // Reserve 2MB for kernel (generous estimate)
        pmm_reserve_range(kernel_phys, 0x200000);
        log(LOG_TYPE_INFO, "[ PMM ] Reserved kernel at 0x%x (2MB)\n", kernel_phys);
    }

    // Reserve first page (NULL pointer protection)
    pmm_reserve_range(0, PAGE_SIZE);

    bitmap_initialized = 1;

    log(LOG_TYPE_INFO, "[ PMM ] Initialized: %d MB total, %d pages free\n",
        (max_addr - phys_base) / (1024 * 1024), free_pages);
}

/**
 * @brief Allocate a single physical page.
 */
uint64_t pmm_alloc_page(void) {
    if (!bitmap_initialized || free_pages == 0) return 0;

    // Simple linear search - O(n) but fine for small systems
    // Start from page 1 to skip NULL page
    for (size_t i = 1; i < MAX_PHYS_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            return page_to_addr(i);
        }
    }

    return 0;
}

/**
 * @brief Free a physical page.
 */
void pmm_free_page(uint64_t addr) {
    if (!bitmap_initialized || addr == 0) return;

    size_t page = addr_to_page(addr);
    if (page == 0 || page >= MAX_PHYS_PAGES) return;

    if (bitmap_test(page)) {
        bitmap_clear(page);
        free_pages++;
    }
}

/**
 * @brief Allocate n contiguous pages.
 */
uint64_t pmm_alloc_pages(size_t n) {
    if (!bitmap_initialized || n == 0 || free_pages < n) return 0;
    if (n > MAX_PHYS_PAGES) return 0;

    size_t consecutive = 0;
    size_t start = 0;

    // Start from page 1 to skip NULL page
    for (size_t i = 1; i < MAX_PHYS_PAGES; i++) {
        if (!bitmap_test(i)) {
            if (consecutive == 0) start = i;
            consecutive++;
            if (consecutive >= n) {
                // Mark all as used
                for (size_t j = start; j < start + n && j < MAX_PHYS_PAGES; j++) {
                    bitmap_set(j);
                }
                free_pages -= n;
                return page_to_addr(start);
            }
        } else {
            consecutive = 0;
        }
    }

    return 0;  // No contiguous block found
}

/**
 * @brief Free n contiguous pages.
 */
void pmm_free_pages(uint64_t addr, size_t n) {
    if (!bitmap_initialized || addr == 0 || n == 0) return;

    size_t start = addr_to_page(addr);
    if (start == 0 || start + n > MAX_PHYS_PAGES) return;

    for (size_t i = start; i < start + n; i++) {
        if (bitmap_test(i)) {
            bitmap_clear(i);
            free_pages++;
        }
    }
}

/**
 * @brief Get count of free pages.
 */
size_t pmm_free_count(void) {
    return free_pages;
}

/**
 * @brief Get total physical memory in bytes.
 */
uint64_t pmm_total_memory(void) {
    return total_pages * PAGE_SIZE;
}
