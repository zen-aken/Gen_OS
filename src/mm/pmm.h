#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

/**
 * @brief Initialize the physical memory manager with Limine memory map.
 * Must be called once before any pmm_alloc/pmm_free operations.
 */
void pmm_init(void);

/**
 * @brief Allocate a single physical page (4KB).
 * @return Physical address of allocated page, or 0 if out of memory.
 */
uint64_t pmm_alloc_page(void);

/**
 * @brief Free a previously allocated physical page.
 * @param addr Physical address of page to free (must be page-aligned).
 */
void pmm_free_page(uint64_t addr);

/**
 * @brief Allocate n contiguous physical pages.
 * @param n Number of pages to allocate.
 * @return Physical address of first page, or 0 if allocation failed.
 */
uint64_t pmm_alloc_pages(size_t n);

/**
 * @brief Free n contiguous physical pages.
 * @param addr Physical address of first page.
 * @param n Number of pages to free.
 */
void pmm_free_pages(uint64_t addr, size_t n);

/**
 * @brief Get total number of free pages available.
 */
size_t pmm_free_count(void);

/**
 * @brief Get total physical memory in bytes.
 */
uint64_t pmm_total_memory(void);
