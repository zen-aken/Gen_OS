#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PTE_PRESENT  0x001
#define PTE_WRITABLE 0x002
#define PTE_USER     0x004
#define PTE_WTHROUGH 0x008
#define PTE_NOCACHE  0x010
#define PTE_ACCESSED 0x020
#define PTE_DIRTY    0x040
#define PTE_HUGEPAGE 0x080
#define PTE_GLOBAL   0x100
#define PTE_NOEXEC   (1ULL << 63)

#define VMM_KERNEL_FLAGS (PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL)
#define VMM_USER_FLAGS   (PTE_PRESENT | PTE_WRITABLE | PTE_USER)
#define VMM_FLAGS_MASK   0xFFF

/**
 * @brief Initialize the virtual memory manager.
 * Sets up kernel page tables and enables higher-half mapping.
 */
void vmm_init(void);

/**
 * @brief Map a physical page to a virtual address.
 * @param vaddr Virtual address (page-aligned).
 * @param paddr Physical address (page-aligned).
 * @param flags Page table entry flags.
 * @return true on success, false on failure.
 */
bool vmm_map(uint64_t vaddr, uint64_t paddr, uint64_t flags);

/**
 * @brief Unmap a virtual address.
 * @param vaddr Virtual address to unmap.
 * @return Physical address that was mapped there, or 0 if not mapped.
 */
uint64_t vmm_unmap(uint64_t vaddr);

/**
 * @brief Allocate a physical page and map it to virtual address.
 * @param vaddr Virtual address to allocate for.
 * @param flags Page table entry flags.
 * @return true on success, false on failure.
 */
bool vmm_alloc(uint64_t vaddr, uint64_t flags);

/**
 * @brief Unmap and free a virtual address.
 * @param vaddr Virtual address to free.
 */
void vmm_free(uint64_t vaddr);

/**
 * @brief Get physical address for a virtual address.
 * @param vaddr Virtual address.
 * @return Physical address, or 0 if not mapped.
 */
uint64_t vmm_get_phys(uint64_t vaddr);

/**
 * @brief Invalidate TLB entry for address.
 */
static inline void vmm_invlpg(uint64_t vaddr) {
    asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

/**
 * @brief Get current PML4 (top-level page table) physical address.
 */
uint64_t vmm_get_cr3(void);

/**
 * @brief Switch to new page table.
 * @param pml4_phys Physical address of new PML4.
 */
void vmm_set_cr3(uint64_t pml4_phys);
