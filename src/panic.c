//* Generic panic handler - architecture-agnostic crash reporting

#include <log.h>
#include <halt.h>
#include <stdint.h>

/**
 * @brief Prints register state from interrupt frame
 */
void panic_dump_frame(uint64_t rip, uint64_t rsp, uint64_t rflags, uint64_t cs, uint64_t ss) {
    log(LOG_TYPE_ERROR, "  RIP: 0x%x  RSP: 0x%x  RFLAGS: 0x%x\n", rip, rsp, rflags);
    log(LOG_TYPE_ERROR, "  CS:  0x%x  SS:  0x%x\n", cs, ss);
}

/**
 * @brief Prints error code interpretation for page fault
 */
void panic_page_fault_code(uint64_t error_code) {
    log(LOG_TYPE_ERROR, "  Error code: 0x%x (", error_code);
    if (error_code & 0x01) log(LOG_TYPE_ERROR, "present ");
    else log(LOG_TYPE_ERROR, "not-present ");
    if (error_code & 0x02) log(LOG_TYPE_ERROR, "write ");
    else log(LOG_TYPE_ERROR, "read ");
    if (error_code & 0x04) log(LOG_TYPE_ERROR, "user ");
    else log(LOG_TYPE_ERROR, "supervisor ");
    if (error_code & 0x08) log(LOG_TYPE_ERROR, "reserved-bit ");
    if (error_code & 0x10) log(LOG_TYPE_ERROR, "fetch ");
    log(LOG_TYPE_ERROR, ")\n");
}

/**
 * @brief Generic panic - prints message and halts
 */
void panic(const char *msg) {
    log(LOG_TYPE_ERROR, "[ PANIC ] %s\n", msg);
    while (1) halt();
}
