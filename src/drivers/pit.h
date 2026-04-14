#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize PIT (Programmable Interval Timer).
 * Sets up channel 0 as periodic IRQ0 at ~1000Hz.
 */
void pit_init(void);

/**
 * @brief Set PIT frequency.
 * @param hz Desired frequency (1-1193180).
 */
void pit_set_frequency(uint32_t hz);

/**
 * @brief Get current tick count.
 * @return Number of timer ticks since init.
 */
uint64_t pit_get_ticks(void);

/**
 * @brief Sleep for specified milliseconds (blocking, polled).
 * @param ms Milliseconds to sleep.
 */
void pit_sleep(uint32_t ms);

struct interrupt_frame;  // Forward declaration

/**
 * @brief IRQ0 handler.
 * Call from timer interrupt handler.
 */
__attribute__((interrupt)) void pit_irq0_handler(struct interrupt_frame *frame);

/**
 * @brief Check if interval elapsed.
 * @param start_tick Starting tick count.
 * @param interval_ms Interval in milliseconds.
 * @return true if interval elapsed.
 */
bool pit_elapsed(uint64_t start_tick, uint32_t interval_ms);
