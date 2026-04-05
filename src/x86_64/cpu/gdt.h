#pragma once

#include <stdint.h>

#define KERNEL_CODE 0x08
#define KERNEL_DATA 0x10

struct GDTEntry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct GDTPtr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void init_gdt();