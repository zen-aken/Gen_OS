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
#include "utils/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "drivers/framebuffer/framebuffer.h"
#include "drivers/ps2_keyboard.h"
#include "drivers/pit.h"

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

    //* Initialize heap allocator
    heap_init();

    //* Test heap
    void *test_ptr = kmalloc(1024);
    if (test_ptr) {
        log(LOG_TYPE_INFO, "[ HEAP ] Allocated 1KB at %p\n", test_ptr);
        *((uint64_t *)test_ptr) = 0xCAFEBABE;
        log(LOG_TYPE_INFO, "[ HEAP ] Wrote test value: 0x%x\n", *((uint64_t *)test_ptr));
        kfree(test_ptr);
        log(LOG_TYPE_INFO, "[ HEAP ] Freed allocation\n");
    }
    log(LOG_TYPE_INFO, "[ HEAP ] Free space: %d KB, Total: %d KB\n",
        heap_free_space() / 1024, heap_total_size() / 1024);

    //* Initialize PIT timer
    pit_init();

    //* Test PIT
    uint64_t ticks_start = pit_get_ticks();
    pit_sleep(100);  // Sleep 100ms
    uint64_t ticks_elapsed = pit_get_ticks() - ticks_start;
    log(LOG_TYPE_INFO, "[ PIT ] Sleep test: %d ticks for 100ms\n", ticks_elapsed);

    //* Initialize keyboard
    keyboard_init();
    log(LOG_TYPE_INFO, "[ KEYBOARD ] Press keys to test...\n");

    //* Enable interrupts for timer and keyboard
    asm volatile("sti");

    log(LOG_TYPE_INFO, "\n[ KERNEL ] All subsystems initialized.\n");
    log(LOG_TYPE_INFO, "[ KERNEL ] Available commands: help, mem, time, reboot\n\n");

    // Simple interactive shell
    char cmd_buf[64];
    int cmd_pos = 0;

    while (1) {
        // Check for keypress
        if (keyboard_has_data()) {
            char c = keyboard_getchar();

            if (c == '\n') {
                cmd_buf[cmd_pos] = '\0';
                log(LOG_TYPE_INFO, "\n");

                // Process command
                if (cmd_pos > 0) {
                    if (strcmp(cmd_buf, "help") == 0) {
                        log(LOG_TYPE_INFO, "Commands:\n");
                        log(LOG_TYPE_INFO, "  help    - Show this help\n");
                        log(LOG_TYPE_INFO, "  mem     - Show memory stats\n");
                        log(LOG_TYPE_INFO, "  time    - Show timer ticks\n");
                        log(LOG_TYPE_INFO, "  reboot  - Reboot system\n");
                    } else if (strcmp(cmd_buf, "mem") == 0) {
                        log(LOG_TYPE_INFO, "Memory:\n");
                        log(LOG_TYPE_INFO, "  PMM free pages: %d\n", pmm_free_count());
                        log(LOG_TYPE_INFO, "  Heap free: %d KB\n", heap_free_space() / 1024);
                        log(LOG_TYPE_INFO, "  Heap total: %d KB\n", heap_total_size() / 1024);
                    } else if (strcmp(cmd_buf, "time") == 0) {
                        log(LOG_TYPE_INFO, "Timer ticks: %d\n", pit_get_ticks());
                    } else if (strcmp(cmd_buf, "reboot") == 0) {
                        log(LOG_TYPE_INFO, "Rebooting...\n");
                        // Triple-fault via invalid IDT
                        asm volatile("lidt (%%rax); int $3" :: "a"(0));
                    } else {
                        log(LOG_TYPE_WARNING, "Unknown command: %s\n", cmd_buf);
                    }
                }

                cmd_pos = 0;
                log(LOG_TYPE_INFO, "> ");
            } else if (c == '\b' && cmd_pos > 0) {
                cmd_pos--;
                log(LOG_TYPE_INFO, "\b \b");
            } else if (c >= 32 && c < 127 && cmd_pos < 63) {
                cmd_buf[cmd_pos++] = c;
                char echo[2] = {c, '\0'};
                log(LOG_TYPE_INFO, echo);
            }
        }

        halt();
    }
}
