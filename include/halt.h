/**
 * @brief Halt the CPU until the next interrupt occurs.
 * This function is architecture-specific and uses the appropriate assembly instruction to halt the CPU.
 * On x86_64, it uses the HLT instruction, and on AArch64, it uses the WFI instruction.
 */
static inline void halt(){
    #ifdef __x86_64__
        asm volatile("hlt");
    #elif __aarch64__
        asm volatile("wfi");
    #endif
}