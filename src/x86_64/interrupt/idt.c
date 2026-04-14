#include "idt.h"
#include "exception.h"
#include <halt.h>
#include <log.h>

//? IDT entry count for x86_64 architecture - typically 256 entries to cover all exceptions and interrupts
#define IDT_ENTRY_COUNT 256

//* IRQ offsets (PIC remapped)
#define IRQ0_OFFSET 32

//* External handlers (from drivers)
extern void pit_irq0_handler(void);
extern __attribute__((interrupt)) void keyboard_irq1_handler(struct interrupt_frame *frame);

//* Code segment selector for the kernel (from GDT) - this should match the value used in the GDT setup
//* Limine uses 0x28 for 64-bit code; after init_gdt() runs, our GDT places it at 0x08
#define KERNEL_CODE_SEGMENT 0x08

//* Interrupt Stack Table index for the kernel (if using IST for certain exceptions)
#define KERNEL_IST 0

struct InterruptDescriptor64 IDT[IDT_ENTRY_COUNT];

uint8_t kernel_exception_attiributes = (1 << 7) | (0 << 5) | (0 << 4) | (0xE); // Present, DPL=0, Type=0xE (Interrupt Gate)

/**
 * @brief Sets an entry in the Interrupt Descriptor Table (IDT) for x86_64 architecture.
 * @param index The index in the IDT to set (0-255).
 * @param addr The address of the interrupt handler function.
 * @param attributes The type and attributes for the IDT entry (e.g., present, DPL, gate type).
 * @param selector The code segment selector in the GDT that the handler will use.
 * @param ist The Interrupt Stack Table offset (0-7) if using IST for this entry, otherwise 0.
 */
void set_idt_entry(size_t index, uint64_t addr, uint8_t attributes, uint16_t selector, uint8_t ist)
{
    struct InterruptDescriptor64 entry;
    entry.offset_1 = addr & 0xFFFF;             // Set offset bits 0..15
    entry.offset_2 = (addr >> 16) & 0xFFFF;     // Set offset bits 16..31
    entry.offset_3 = (addr >> 32) & 0xFFFFFFFF; // Set offset bits 32..63
    entry.type_attributes = attributes;         // Set the type and attributes
    entry.selector = selector;                  // Code segment selector in GDT
    entry.ist = ist;                            // Interrupt Stack Table offset
    entry.zero = 0;                             // Reserved

    IDT[index] = entry;
}

// Default exception handler for unhandled exceptions - logs the error and halts the system
__attribute__((interrupt)) void default_exception_handler(struct interrupt_frame* frame)
{
    log(LOG_TYPE_ERROR, "[ INTERRUPT ] Unhandled exception occurred! Halting the system.\n");
    while (1)
    {
        halt();
    }
}

void init_idt()
{
    for (int i = 0; i < IDT_ENTRY_COUNT; i++)
    {
        set_idt_entry(i, (uint64_t)default_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    }
    set_idt_entry(0, (uint64_t)kill_app_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(1, (uint64_t)debugger_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(2, (uint64_t)nmi_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(3, (uint64_t)debugger_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(4, (uint64_t)kill_app_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(5, (uint64_t)kill_app_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(6, (uint64_t)kill_app_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(7, (uint64_t)fpu_restore_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(8, (uint64_t)double_fault_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    //! [RESERVED] set_idt_entry(9, (uint64_t)0, 0, 0, 0);
    set_idt_entry(10, (uint64_t)invalid_tss_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(11, (uint64_t)segment_not_present_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(12, (uint64_t)stack_segment_fault_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(13, (uint64_t)general_protection_fault_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(14, (uint64_t)page_fault_exception_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    //! [RESERVED] set_idt_entry(15, (uint64_t)0, 0, 0, 0);

    // IRQ handlers (remapped to IDT entries 32-47)
    set_idt_entry(IRQ0_OFFSET + 0, (uint64_t)pit_irq0_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);
    set_idt_entry(IRQ0_OFFSET + 1, (uint64_t)keyboard_irq1_handler, kernel_exception_attiributes, KERNEL_CODE_SEGMENT, KERNEL_IST);

    struct IDTR idtr;
    idtr.addr = (uint64_t)IDT;
    idtr.size = 256 * sizeof(struct InterruptDescriptor64) - 1;

    asm volatile("lidt %0" : : "m"(idtr));

    log(LOG_TYPE_INFO, "[ IDT ] IDT initialized with %d entries\n", 256);
}