#include "exception.h"
#include <log.h>
#include <halt.h>
#include "panic.h"

// idt id: 0, 4, 5, 6
__attribute__((interrupt)) void kill_app_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Fatal exception (ID 0/4/5/6)!\n");
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 1, 3
__attribute__((interrupt)) void debugger_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Debug/Breakpoint exception (ID 1/3) - RIP: 0x%x\n", frame->rip);
    // Debug exceptions are often recoverable; continue execution
}

// idt id: 2
__attribute__((interrupt)) void nmi_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Non-Maskable Interrupt (ID 2)!\n");
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 7
__attribute__((interrupt)) void fpu_restore_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Device Not Available / FPU exception (ID 7) - RIP: 0x%x\n", frame->rip);
    // Can be recovered by saving/restoring FPU state
    while (1) halt();
}

// idt id: 8 (error_code pushed by CPU)
__attribute__((interrupt)) void double_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Double Fault (ID 8)! Error code: 0x%x\n", error_code);
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 10 (error_code pushed by CPU)
__attribute__((interrupt)) void invalid_tss_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Invalid TSS (ID 10)! Selector: 0x%x\n", error_code);
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 11 (error_code pushed by CPU)
__attribute__((interrupt)) void segment_not_present_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Segment Not Present (ID 11)! Selector: 0x%x\n", error_code);
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 12 (error_code pushed by CPU)
__attribute__((interrupt)) void stack_segment_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Stack Segment Fault (ID 12)!\n");
    if (error_code != 0) {
        log(LOG_TYPE_ERROR, "  Selector: 0x%x, External: %s\n",
            error_code & 0xFFF8,
            (error_code & 0x01) ? "yes" : "no");
    } else {
        log(LOG_TYPE_ERROR, "  Stack limit exceeded or invalid stack segment\n");
    }
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 13 (error_code pushed by CPU)
__attribute__((interrupt)) void general_protection_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] General Protection Fault (ID 13)!\n");
    if (error_code != 0) {
        uint8_t ext = error_code & 0x01;
        uint8_t idt = (error_code >> 1) & 0x01;
        uint8_t ti = (error_code >> 2) & 0x01;
        uint16_t sel = (error_code >> 3) & 0x1FFF;
        log(LOG_TYPE_ERROR, "  External: %s, IDT: %s, TI: %s\n",
            ext ? "yes" : "no",
            idt ? "yes" : "no",
            ti ? "LDT" : "GDT");
        log(LOG_TYPE_ERROR, "  Segment selector: 0x%x\n", sel);
    } else {
        log(LOG_TYPE_ERROR, "  Null selector or I/O permission violation\n");
    }
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}

// idt id: 14 (error_code pushed by CPU)
__attribute__((interrupt)) void page_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    uint64_t cr2;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Page Fault (ID 14)! Faulting address: 0x%x\n", cr2);
    panic_page_fault_code(error_code);
    panic_dump_frame(frame->rip, frame->rsp, frame->rflags, frame->cs, frame->ss);
    while (1) halt();
}
