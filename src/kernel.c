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
#include "drivers/framebuffer/framebuffer.h"

void kernel_main()
{
    init_framebuffer(framebuffer_request.response);
    init_idt();
    init_gdt();

    // Interrupt test code
    // Division by zero (0)
    // volatile int x = 1 / 0;

    // Breakpoint (3)
    // asm volatile("int3");

    // Overflow (4)
    // asm volatile("into");

    // Invalid opcode (6)
    // asm volatile("ud2");

    // Page fault (14)
    volatile int *ptr = (int *)0xdeadbeef;
    *ptr = 42;

    while (1)
    {
        halt();
    }
}