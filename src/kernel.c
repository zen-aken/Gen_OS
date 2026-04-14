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
