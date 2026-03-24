// ARCH based includes
#ifdef __x86_64__
#include <x86_64/limine/limine_requests.h>
#endif

#ifdef __aarch64__
#include <aarch64/limine/limine_requests.h>
#endif

// common
#include "framebuffer/framebuffer.h"

void halt(){
    #ifdef __x86_64__
        asm volatile("hlt");
    #elif __aarch64__
        asm volatile("wfi");
    #endif
}

void kernel_main(){
    framebuffer_init(framebuffer_request.response);
    while (1)
    {
        halt();
    }
}