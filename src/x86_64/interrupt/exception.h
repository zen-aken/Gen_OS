#pragma once
#include <stdint.h>

struct interrupt_frame
{
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));

__attribute__((interrupt)) void kill_app_exception_handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void debugger_exception_handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void nmi_exception_handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void fpu_restore_exception_handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void double_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void invalid_tss_exception_handler(struct interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void segment_not_present_exception_handler(struct interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void stack_segment_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void general_protection_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code);
__attribute__((interrupt)) void page_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code);