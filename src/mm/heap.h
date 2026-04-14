#pragma once

#include <stdint.h>
#include <stddef.h>

#define HEAP_START_VADDR 0xFFFF900000000000ULL  // High kernel heap area

/**
 * @brief Initialize kernel heap allocator.
 * Sets up initial heap pages.
 */
void heap_init(void);

/**
 * @brief Allocate memory from kernel heap.
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *kmalloc(size_t size);

/**
 * @brief Allocate aligned memory.
 * @param size Number of bytes.
 * @param align Alignment (must be power of 2, >= 8).
 * @return Aligned pointer or NULL.
 */
void *kmalloc_aligned(size_t size, size_t align);

/**
 * @brief Allocate and zero memory.
 * @param size Number of bytes.
 * @return Zeroed pointer or NULL.
 */
void *kzalloc(size_t size);

/**
 * @brief Free allocated memory.
 * @param ptr Pointer to free (NULL is safe).
 */
void kfree(void *ptr);

/**
 * @brief Reallocate memory.
 * @param ptr Previous allocation (NULL = new alloc).
 * @param new_size New size.
 * @return New pointer or NULL on failure (original intact).
 */
void *krealloc(void *ptr, size_t new_size);

/**
 * @brief Get total heap size.
 */
size_t heap_total_size(void);

/**
 * @brief Get free heap space.
 */
size_t heap_free_space(void);
