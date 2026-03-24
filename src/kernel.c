#include <x86_64/limine/limine_requests.h>
#include <aarch64/limine/limine_requests.h>

void halt(){
    #ifdef __x86_64__
        asm volatile("hlt");
    #elif __aarch64__
        asm volatile("wfi");
    #endif
}

void kernel_main(){
    while (1)
    {
        halt();
    }
}