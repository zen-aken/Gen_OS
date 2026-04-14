#include "gdt.h"

struct GDTEntry gdt[3];

void set_gdt_entry(int i, uint8_t access, uint8_t flags)
{
    gdt[i].limit_low = 0;
    gdt[i].base_low = 0;
    gdt[i].base_mid = 0;
    gdt[i].access = access;
    gdt[i].granularity = (flags & 0xF0);
    gdt[i].base_high = 0;
}

void reload_segments()
{
    asm volatile(
        "mov %[data], %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"

        "pushq %[code]\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"

        "1:\n"
        :
        : [code] "i"(KERNEL_CODE), [data] "i"(KERNEL_DATA)
        : "rax", "memory");
}

void init_gdt()
{
    set_gdt_entry(0, 0, 0);
    set_gdt_entry(1, 0x9A, 0x20);
    set_gdt_entry(2, 0x92, 0x00);

    struct GDTPtr gdt_ptr;
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    asm volatile("cli");
    asm volatile("lgdt %0" : : "m"(gdt_ptr));
    reload_segments();
}