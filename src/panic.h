#pragma once

#include <stdint.h>

void panic_dump_frame(uint64_t rip, uint64_t rsp, uint64_t rflags, uint64_t cs, uint64_t ss);
void panic_page_fault_code(uint64_t error_code);
void panic(const char *msg);
