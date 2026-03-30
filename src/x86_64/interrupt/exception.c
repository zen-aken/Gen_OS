#include "exception.h"
#include <log.h>
#include <halt.h>

// idt id: 0, 4, 5, 6
__attribute__((interrupt)) void kill_app_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Fatal exception occurred in application! Terminating the application.\n");
    while (1) {
        halt();
    }
} 

// idt id: 1, 3
__attribute__((interrupt)) void debugger_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Debugger exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 2
__attribute__((interrupt)) void nmi_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] NMI exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 7
__attribute__((interrupt)) void fpu_restore_exception_handler(struct interrupt_frame* frame) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] FPU restore exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 8
__attribute__((interrupt)) void double_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Double fault exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 10
__attribute__((interrupt)) void invalid_tss_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Invalid TSS exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 11
__attribute__((interrupt)) void segment_not_present_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Segment not present exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 12
__attribute__((interrupt)) void stack_segment_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Stack segment fault exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 

// idt id: 13
__attribute__((interrupt)) void general_protection_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] General protection fault exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
}

// idt id: 14
__attribute__((interrupt)) void page_fault_exception_handler(struct interrupt_frame* frame, uint64_t error_code) {
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Page fault exception occurred! Halting the system.\n");
    while (1) {
        halt();
    }
} 