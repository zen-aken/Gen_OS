#pragma once
#include <stdint.h>
#include <stddef.h>

struct InterruptDescriptor64 {
   uint16_t offset_1;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_2;        // offset bits 16..31
   uint32_t offset_3;        // offset bits 32..63
   uint32_t zero;            // reserved
} __attribute__((packed));

struct IDTR
{
    uint16_t size;
    uint64_t addr;
} __attribute__((packed));




void set_idt_entry(size_t index, uint64_t addr, uint8_t attributes, uint16_t selector, uint8_t ist);
void init_idt();