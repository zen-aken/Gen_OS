#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

/**
 * @brief Initialize PS/2 keyboard controller.
 */
void keyboard_init(void);

/**
 * @brief Check if a scancode is available.
 */
bool keyboard_has_data(void);

/**
 * @brief Read raw scancode from keyboard.
 * @return Scancode byte, or 0 if no data.
 */
uint8_t keyboard_read_scancode(void);

/**
 * @brief Read ASCII character (blocking with simple translation).
 * @return ASCII char, or 0 if no translation.
 */
char keyboard_getchar(void);

/**
 * @brief Get string up to newline (blocking).
 * @param buf Buffer to fill.
 * @param max_len Maximum length including null terminator.
 */
void keyboard_gets(char *buf, int max_len);

/**
 * @brief Check if keyboard buffer has a full line ready.
 */
bool keyboard_line_ready(void);

/**
 * @brief Read buffered line (non-blocking, returns false if not ready).
 */
bool keyboard_read_line(char *buf, int max_len);

struct interrupt_frame;  // Forward declaration

/**
 * @brief IRQ1 handler entry point.
 */
__attribute__((interrupt)) void keyboard_irq1_handler(struct interrupt_frame *frame);
