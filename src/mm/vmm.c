//* Virtual Memory Manager - x86_64 4-level paging
//  Manages page tables: PML4 -> PDPT -> PD -> PT

//* Virtual Memory Manager - x86_64 4-level paging
//  Manages page tables: PML4 -> PDPT -> PD -> PT

#include "vmm.h"
#include "pmm.h"
#include <limine.h>
#include <log.h>

// Simple memset for zeroing page tables (no libc)
static void *vmm_memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

#define PML4_INDEX(vaddr) (((vaddr) >> 39) & 0x1FF)
#define PDPT_INDEX(vaddr) (((vaddr) >> 30) & 0x1FF)
#define PD_INDEX(vaddr)   (((vaddr) >> 21) & 0x1FF)
#define PT_INDEX(vaddr)   (((vaddr) >> 12) & 0x1FF)

#define ENTRIES_PER_TABLE 512
#define RECURSIVE_PML4_ENTRY 510

// External Limine HHDM request
extern volatile struct limine_hhdm_request hhdm_request;

static uint64_t *kernel_pml4 = NULL;
static uint64_t hhdm_offset = 0;

/**
 * @brief Get HHDM offset - physical addresses are accessed at phys + hhdm_offset
 */
static inline uint64_t phys_to_hhdm(uint64_t phys) {
    return phys + hhdm_offset;
}

/**
 * @brief Allocate a new page table page and zero it.
 */
static uint64_t *alloc_table(void) {
    uint64_t phys = pmm_alloc_page();
    if (phys == 0) return NULL;
    
    uint64_t *virt = (uint64_t *)phys_to_hhdm(phys);
    vmm_memset(virt, 0, PAGE_SIZE);
    return virt;
}

/**
 * @brief Get or create PDPT for given PML4 index.
 */
static uint64_t *get_pdpt(uint64_t pml4_idx, bool create) {
    uint64_t pml4_entry = kernel_pml4[pml4_idx];
    
    if (!(pml4_entry & PTE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t *new_pdpt = alloc_table();
        if (!new_pdpt) return NULL;
        
        uint64_t pdpt_phys = (uint64_t)new_pdpt - hhdm_offset;
        kernel_pml4[pml4_idx] = pdpt_phys | VMM_KERNEL_FLAGS;
        return new_pdpt;
    }
    
    uint64_t pdpt_phys = pml4_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    return (uint64_t *)phys_to_hhdm(pdpt_phys);
}

/**
 * @brief Get or create PD for given PDPT index.
 */
static uint64_t *get_pd(uint64_t pml4_idx, uint64_t pdpt_idx, bool create) {
    uint64_t *pdpt = get_pdpt(pml4_idx, create);
    if (!pdpt) return NULL;
    
    uint64_t pdpt_entry = pdpt[pdpt_idx];
    
    if (!(pdpt_entry & PTE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t *new_pd = alloc_table();
        if (!new_pd) return NULL;
        
        uint64_t pd_phys = (uint64_t)new_pd - hhdm_offset;
        pdpt[pdpt_idx] = pd_phys | VMM_KERNEL_FLAGS;
        return new_pd;
    }
    
    // Check if this is a 1GB huge page
    if (pdpt_entry & PTE_HUGEPAGE) {
        return NULL;  // Cannot map in huge page region
    }
    
    uint64_t pd_phys = pdpt_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    return (uint64_t *)phys_to_hhdm(pd_phys);
}

/**
 * @brief Get or create PT for given PD index.
 */
static uint64_t *get_pt(uint64_t pml4_idx, uint64_t pdpt_idx, uint64_t pd_idx, bool create) {
    uint64_t *pd = get_pd(pml4_idx, pdpt_idx, create);
    if (!pd) return NULL;
    
    uint64_t pd_entry = pd[pd_idx];
    
    if (!(pd_entry & PTE_PRESENT)) {
        if (!create) return NULL;
        
        uint64_t *new_pt = alloc_table();
        if (!new_pt) return NULL;
        
        uint64_t pt_phys = (uint64_t)new_pt - hhdm_offset;
        pd[pd_idx] = pt_phys | VMM_KERNEL_FLAGS;
        return new_pt;
    }
    
    // Check if 2MB huge page
    if (pd_entry & PTE_HUGEPAGE) {
        return NULL;
    }
    
    uint64_t pt_phys = pd_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    return (uint64_t *)phys_to_hhdm(pt_phys);
}

/**
 * @brief Initialize VMM - clone Limine's page tables for kernel use.
 */
void vmm_init(void) {
    // Get HHDM offset
    hhdm_offset = 0xFFFF800000000000ULL;
    if (hhdm_request.response) {
        hhdm_offset = hhdm_request.response->offset;
    }
    
    log(LOG_TYPE_INFO, "[ VMM ] HHDM offset: 0x%x\n", hhdm_offset);
    
    // Read current CR3 to find Limine's PML4
    uint64_t current_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(current_cr3));
    
    uint64_t limine_pml4_phys = current_cr3 & 0x000FFFFFFFFFF000ULL;
    uint64_t *limine_pml4 = (uint64_t *)phys_to_hhdm(limine_pml4_phys);
    
    log(LOG_TYPE_INFO, "[ VMM ] Limine PML4 at phys 0x%x\n", limine_pml4_phys);
    
    // Allocate our own PML4
    kernel_pml4 = alloc_table();
    if (!kernel_pml4) {
        log(LOG_TYPE_ERROR, "[ VMM ] Failed to allocate kernel PML4!\n");
        return;
    }
    
    // Copy kernel half entries (256-511) from Limine
    // These map higher-half kernel space
    for (int i = 256; i < ENTRIES_PER_TABLE; i++) {
        kernel_pml4[i] = limine_pml4[i];
    }
    
    // Keep Limine's recursive mapping if present (for debugging)
    if (limine_pml4[RECURSIVE_PML4_ENTRY] & PTE_PRESENT) {
        kernel_pml4[RECURSIVE_PML4_ENTRY] = limine_pml4[RECURSIVE_PML4_ENTRY];
    }
    
    // Switch to our new page tables
    uint64_t new_pml4_phys = (uint64_t)kernel_pml4 - hhdm_offset;
    vmm_set_cr3(new_pml4_phys);
    
    log(LOG_TYPE_INFO, "[ VMM ] Initialized with new PML4 at phys 0x%x\n", new_pml4_phys);
}

/**
 * @brief Map physical page to virtual address.
 */
bool vmm_map(uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    if (!kernel_pml4) return false;
    if (vaddr & (PAGE_SIZE - 1)) return false;  // Must be aligned
    if (paddr & (PAGE_SIZE - 1)) return false;
    
    uint64_t pml4_idx = PML4_INDEX(vaddr);
    uint64_t pdpt_idx = PDPT_INDEX(vaddr);
    uint64_t pd_idx = PD_INDEX(vaddr);
    uint64_t pt_idx = PT_INDEX(vaddr);
    
    // Kernel PML4 entries 256+ are sacred - don't modify them
    if (pml4_idx >= 256) {
        log(LOG_TYPE_WARNING, "[ VMM ] Refusing to map in kernel half: 0x%x\n", vaddr);
        return false;
    }
    
    uint64_t *pt = get_pt(pml4_idx, pdpt_idx, pd_idx, true);
    if (!pt) return false;
    
    // Set the page table entry
    pt[pt_idx] = (paddr & 0x000FFFFFFFFFF000ULL) | (flags & VMM_FLAGS_MASK) | PTE_PRESENT;
    
    // Flush TLB
    vmm_invlpg(vaddr);
    
    return true;
}

/**
 * @brief Unmap virtual address.
 */
uint64_t vmm_unmap(uint64_t vaddr) {
    if (!kernel_pml4) return 0;
    if (vaddr & (PAGE_SIZE - 1)) return 0;
    
    uint64_t pml4_idx = PML4_INDEX(vaddr);
    uint64_t pdpt_idx = PDPT_INDEX(vaddr);
    uint64_t pd_idx = PD_INDEX(vaddr);
    uint64_t pt_idx = PT_INDEX(vaddr);
    
    uint64_t *pt = get_pt(pml4_idx, pdpt_idx, pd_idx, false);
    if (!pt) return 0;
    
    uint64_t entry = pt[pt_idx];
    if (!(entry & PTE_PRESENT)) return 0;
    
    uint64_t paddr = entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    pt[pt_idx] = 0;
    
    vmm_invlpg(vaddr);
    
    return paddr;
}

/**
 * @brief Allocate physical page and map it.
 */
bool vmm_alloc(uint64_t vaddr, uint64_t flags) {
    uint64_t paddr = pmm_alloc_page();
    if (paddr == 0) return false;
    
    if (!vmm_map(vaddr, paddr, flags)) {
        pmm_free_page(paddr);
        return false;
    }

    // Zero the new page
    vmm_memset((void *)vaddr, 0, PAGE_SIZE);

    return true;
}

/**
 * @brief Unmap and free physical page.
 */
void vmm_free(uint64_t vaddr) {
    uint64_t paddr = vmm_unmap(vaddr);
    if (paddr) {
        pmm_free_page(paddr);
    }
}

/**
 * @brief Get physical address mapped at virtual address.
 */
uint64_t vmm_get_phys(uint64_t vaddr) {
    if (!kernel_pml4) return 0;
    
    uint64_t pml4_idx = PML4_INDEX(vaddr);
    uint64_t pdpt_idx = PDPT_INDEX(vaddr);
    uint64_t pd_idx = PD_INDEX(vaddr);
    uint64_t pt_idx = PT_INDEX(vaddr);
    
    uint64_t pml4_entry = kernel_pml4[pml4_idx];
    if (!(pml4_entry & PTE_PRESENT)) return 0;
    
    // Handle 512GB huge pages (rare)
    if (pml4_entry & PTE_HUGEPAGE) {
        return (pml4_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL) | (vaddr & 0x7FFFFFFFFFFF);
    }
    
    uint64_t pdpt_phys = pml4_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    uint64_t *pdpt = (uint64_t *)phys_to_hhdm(pdpt_phys);
    uint64_t pdpt_entry = pdpt[pdpt_idx];
    if (!(pdpt_entry & PTE_PRESENT)) return 0;
    
    // Handle 1GB huge pages
    if (pdpt_entry & PTE_HUGEPAGE) {
        return (pdpt_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL) | (vaddr & 0x3FFFFFFF);
    }
    
    uint64_t pd_phys = pdpt_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    uint64_t *pd = (uint64_t *)phys_to_hhdm(pd_phys);
    uint64_t pd_entry = pd[pd_idx];
    if (!(pd_entry & PTE_PRESENT)) return 0;
    
    // Handle 2MB huge pages
    if (pd_entry & PTE_HUGEPAGE) {
        return (pd_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL) | (vaddr & 0x1FFFFF);
    }
    
    uint64_t pt_phys = pd_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL;
    uint64_t *pt = (uint64_t *)phys_to_hhdm(pt_phys);
    uint64_t pt_entry = pt[pt_idx];
    if (!(pt_entry & PTE_PRESENT)) return 0;
    
    return (pt_entry & ~VMM_FLAGS_MASK & 0x000FFFFFFFFFF000ULL) | (vaddr & 0xFFF);
}

/**
 * @brief Read current CR3.
 */
uint64_t vmm_get_cr3(void) {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3 & 0x000FFFFFFFFFF000ULL;
}

/**
 * @brief Switch page tables.
 */
void vmm_set_cr3(uint64_t pml4_phys) {
    asm volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
}
