// ARCH based includes
#ifdef __x86_64__
#include "x86_64/limine/limine_requests.h"
#include "x86_64/interrupt/idt.h"
#include "x86_64/cpu/gdt.h"
#endif

#ifdef __aarch64__
#include "aarch64/limine/limine_requests.h"
#endif

//* COMMON
#include <halt.h>
#include <log.h>
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "drivers/framebuffer/framebuffer.h"

#define KERNEL_VERSION "0.1.0"
#define KERNEL_NAME "Gen_OS"

void kernel_main()
{
    init_framebuffer(framebuffer_request.response);
    init_gdt();
    init_idt();

    //* Boot banner
    clear_screen();
    log(LOG_TYPE_INFO, "========================================\n");
    log(LOG_TYPE_INFO, "  %s Kernel v%s\n", KERNEL_NAME, KERNEL_VERSION);
    log(LOG_TYPE_INFO, "  Build date: %s %s\n", __DATE__, __TIME__);
    log(LOG_TYPE_INFO, "========================================\n\n");

    //* Initialize physical memory manager
    pmm_init();

    //* Test PMM
    uint64_t test_page = pmm_alloc_page();
    if (test_page != 0) {
        log(LOG_TYPE_INFO, "[ PMM ] Test page allocated: 0x%x\n", test_page);
        pmm_free_page(test_page);
        log(LOG_TYPE_INFO, "[ PMM ] Test page freed successfully\n");
    }
    log(LOG_TYPE_INFO, "[ PMM ] Free pages available: %d\n", pmm_free_count());

    //* Initialize virtual memory manager
    log(LOG_TYPE_INFO, "[ VMM ] Initializing virtual memory...\n");
    vmm_init();

    //* Test VMM - map a page in user half
    uint64_t test_vaddr = 0x1000000000ULL;  // User space address (above 4GB)
    log(LOG_TYPE_INFO, "[ VMM ] Testing allocation at vaddr 0x%x\n", test_vaddr);

    if (vmm_alloc(test_vaddr, VMM_KERNEL_FLAGS)) {
        log(LOG_TYPE_INFO, "[ VMM ] Successfully allocated and mapped page\n");

        // Write test pattern
        uint32_t *test_ptr = (uint32_t *)test_vaddr;
        *test_ptr = 0xDEADBEEF;

        if (*test_ptr == 0xDEADBEEF) {
            log(LOG_TYPE_INFO, "[ VMM ] Write/read test PASSED (0x%x)\n", *test_ptr);
        } else {
            log(LOG_TYPE_ERROR, "[ VMM ] Write/read test FAILED!\n");
        }

        // Get physical address
        uint64_t phys = vmm_get_phys(test_vaddr);
        log(LOG_TYPE_INFO, "[ VMM ] Virtual 0x%x -> Physical 0x%x\n", test_vaddr, phys);

        // Unmap and free
        vmm_free(test_vaddr);
        log(LOG_TYPE_INFO, "[ VMM ] Page unmapped and freed\n");
    } else {
        log(LOG_TYPE_ERROR, "[ VMM ] Failed to allocate page!\n");
    }

    log(LOG_TYPE_INFO, "[ KERNEL ] Initialization complete.\n");
    log(LOG_TYPE_INFO, "[ KERNEL ] Press any key to test interrupts...\n");

    // Interrupt test code (commented out by default)
    // Division by zero (0)
    // volatile int x = 1 / 0;

    // Breakpoint (3)
    // asm volatile("int3");

    // Overflow (4)
    // asm volatile("into");

    // Invalid opcode (6)
    // asm volatile("ud2");

    // Page fault (14)
    // volatile int *ptr = (int *)0xdeadbeef;
    // *ptr = 42;

    while (1)
    {
        halt();
    }
}
