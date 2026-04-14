#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

/**
 * @brief Initialize the physical memory manager with Limine memory map.
 * Scans memory map and builds free page stack. Supports full 64-bit addresses.
 */
void pmm_init(void);

/**
 * @brief Allocate a single physical page.
 * @return Physical address of allocated page, or 0 if out of memory.
 *         Returned address is always 4KB aligned and below 4GB initially,
 *         but 64-bit clean for higher addresses.
 */
uint64_t pmm_alloc_page(void);

/**
 * @brief Free a physical page back to the pool.
 * @param addr Physical address of page to free (must be page-aligned, non-zero).
 */
void pmm_free_page(uint64_t addr);

/**
 * @brief Allocate n contiguous physical pages.
 * @param n Number of pages.
 * @return Physical address of first page, or 0 if allocation failed.
 * @note Contiguous allocation is expensive (O(N) scan). Use only when necessary.
 */
uint64_t pmm_alloc_pages(size_t n);

/**
 * @brief Free n contiguous pages.
 * @param addr Physical address of first page.
 * @param n Number of pages.
 */
void pmm_free_pages(uint64_t addr, size_t n);

/**
 * @brief Get number of free pages available.
 */
size_t pmm_free_count(void);

/**
 * @brief Get total usable physical memory in bytes.
 */
uint64_t pmm_total_memory(void);

/**
 * @brief Convert page number to physical address.
 */
static inline uint64_t pmm_page_to_addr(size_t page_num) {
    return (uint64_t)page_num << PAGE_SHIFT;
}

/**
 * @brief Convert physical address to page number.
 */
static inline size_t pmm_addr_to_page(uint64_t addr) {
    return addr >> PAGE_SHIFT;
}
