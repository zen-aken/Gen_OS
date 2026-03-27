// ARCH based includes
#ifdef __x86_64__
#include <x86_64/limine/limine_requests.h>
#endif

#ifdef __aarch64__
#include <aarch64/limine/limine_requests.h>
#endif

//* COMMON
#include "drivers/framebuffer/framebuffer.h"
#include <log.h>

void halt(){
    #ifdef __x86_64__
        asm volatile("hlt");
    #elif __aarch64__
        asm volatile("wfi");
    #endif
}

void kernel_main(){
    framebuffer_init(framebuffer_request.response);
    int x = 2025;
    print("merhaba %d", x);
    while (1)
    {
        halt();
    }
}