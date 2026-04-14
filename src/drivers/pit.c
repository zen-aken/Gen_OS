//* PIT (Programmable Interval Timer) - IRQ0 timer driver

//* PIT (Programmable Interval Timer) - IRQ0 timer driver

#include "pit.h"
#include <log.h>
#include <stdint.h>
#include "x86_64/interrupt/interrupt_frame.h"

#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

#define PIT_FREQUENCY   1193180

static volatile uint64_t tick_count = 0;
static uint32_t current_frequency = 0;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

// Send EOI to PIC
static inline void pic_eoi(void) {
    outb(0x20, 0x20);  // Master PIC EOI
}

void pit_set_frequency(uint32_t hz) {
    if (hz == 0 || hz > PIT_FREQUENCY) return;

    uint32_t divisor = PIT_FREQUENCY / hz;
    if (divisor > 65535) divisor = 65535;
    if (divisor == 0) divisor = 1;

    current_frequency = hz;

    // Command: channel 0, lobyte/hibyte, mode 3 (square wave)
    outb(PIT_COMMAND, 0x36);

    // Send divisor
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

void pit_init(void) {
    // Remap PIC first
    // Save masks
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    // Start initialization
    outb(0x20, 0x11);  // ICW1
    outb(0xA0, 0x11);  // ICW1
    outb(0x21, 0x20);  // ICW2 - Master vector offset 32
    outb(0xA1, 0x28);  // ICW2 - Slave vector offset 40
    outb(0x21, 0x04);  // ICW3 - Slave at IRQ2
    outb(0xA1, 0x02);  // ICW3
    outb(0x21, 0x01);  // ICW4 - 8086 mode
    outb(0xA1, 0x01);  // ICW4

    // Restore masks - unmask IRQ0 and IRQ1, mask others
    outb(0x21, 0xFC);  // Timer (IRQ0) and Keyboard (IRQ1) enabled
    outb(0xA1, a2);

    // Default to ~1000Hz
    pit_set_frequency(1000);
    tick_count = 0;

    log(LOG_TYPE_INFO, "[ PIT ] Initialized at %d Hz, PIC remapped\n", current_frequency);
}

__attribute__((interrupt)) void pit_irq0_handler(struct interrupt_frame *frame) {
    tick_count++;
    pic_eoi();  // Acknowledge interrupt
}

uint64_t pit_get_ticks(void) {
    return tick_count;
}

bool pit_elapsed(uint64_t start_tick, uint32_t interval_ms) {
    uint64_t elapsed_ticks = tick_count - start_tick;
    uint64_t needed_ticks = (uint64_t)interval_ms * current_frequency / 1000;
    return elapsed_ticks >= needed_ticks;
}

void pit_sleep(uint32_t ms) {
    uint64_t start = tick_count;
    uint64_t needed = (uint64_t)ms * current_frequency / 1000;

    while ((tick_count - start) < needed) {
        // Halt until next interrupt
        asm volatile("hlt");
    }
}
