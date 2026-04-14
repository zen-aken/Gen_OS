//* PS/2 Keyboard Driver - Polling and IRQ1 handler

#include "ps2_keyboard.h"
#include <log.h>
#include <stdint.h>
#include "x86_64/interrupt/interrupt_frame.h"

// Simple scancode to ASCII map (US layout, set 1)
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0,   ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static volatile uint8_t key_buffer[256];
static volatile int buffer_head = 0;
static volatile int buffer_tail = 0;
static volatile bool shift_pressed = false;
static volatile bool ctrl_pressed = false;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" :: "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void io_wait(void) {
    asm volatile("outb %%al, $0x80" :: "a"(0));
}

// Send EOI to PIC
static inline void pic_eoi(void) {
    outb(0x20, 0x20);  // Master PIC EOI
}

void keyboard_init(void) {
    // Drain any pending data
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    log(LOG_TYPE_INFO, "[ PS2 ] Keyboard initialized\n");
}

bool keyboard_has_data(void) {
    return inb(0x64) & 0x01;
}

uint8_t keyboard_read_scancode(void) {
    if (keyboard_has_data()) {
        return inb(0x60);
    }
    return 0;
}

static void buffer_push(char c) {
    int next = (buffer_head + 1) & 0xFF;
    if (next != buffer_tail) {
        key_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

static char buffer_pop(void) {
    if (buffer_head == buffer_tail) return 0;
    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) & 0xFF;
    return c;
}

void keyboard_process_scancode(uint8_t scancode) {
    // Handle special keys
    if (scancode == 0x2A || scancode == 0x36) {  // LShift or RShift pressed
        shift_pressed = true;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {  // Shift released
        shift_pressed = false;
        return;
    }
    if (scancode == 0x1D) {  // Ctrl pressed
        ctrl_pressed = true;
        return;
    }
    if (scancode == 0x9D) {  // Ctrl released
        ctrl_pressed = false;
        return;
    }

    // Release codes (ignore)
    if (scancode & 0x80) return;

    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c) {
            if (shift_pressed) {
                // Simple shift conversion
                if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
                else if (c == '1') c = '!';
                else if (c == '2') c = '@';
                else if (c == '3') c = '#';
                else if (c == '4') c = '$';
                else if (c == '5') c = '%';
                else if (c == '6') c = '^';
                else if (c == '7') c = '&';
                else if (c == '8') c = '*';
                else if (c == '9') c = '(';
                else if (c == '0') c = ')';
            }

            if (ctrl_pressed && c >= 'a' && c <= 'z') {
                c = c - 'a' + 1;  // Control characters
            }

            buffer_push(c);
        }
    }
}

__attribute__((interrupt)) void keyboard_irq1_handler(struct interrupt_frame *frame) {
    uint8_t scancode = inb(0x60);
    keyboard_process_scancode(scancode);
    pic_eoi();
}

char keyboard_getchar(void) {
    // Poll until we have a character
    while (buffer_head == buffer_tail) {
        if (keyboard_has_data()) {
            keyboard_process_scancode(keyboard_read_scancode());
        }
    }
    return buffer_pop();
}

void keyboard_gets(char *buf, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = keyboard_getchar();
        if (c == '\n') {
            buf[i] = '\0';
            return;
        } else if (c == '\b' && i > 0) {
            i--;
            log(LOG_TYPE_INFO, "\b \b");  // Visual backspace
        } else if (c >= 32 && c < 127) {
            buf[i++] = c;
            // Echo
            char echo[2] = {c, '\0'};
            log(LOG_TYPE_INFO, echo);
        }
    }
    buf[max_len - 1] = '\0';
}

bool keyboard_line_ready(void) {
    // Scan buffer for newline
    int pos = buffer_tail;
    while (pos != buffer_head) {
        if (key_buffer[pos] == '\n') return true;
        pos = (pos + 1) & 0xFF;
    }
    return false;
}

bool keyboard_read_line(char *buf, int max_len) {
    if (!keyboard_line_ready()) return false;

    int i = 0;
    while (i < max_len - 1 && buffer_tail != buffer_head) {
        char c = buffer_pop();
        buf[i++] = c;
        if (c == '\n') {
            buf[i] = '\0';
            return true;
        }
    }
    buf[max_len - 1] = '\0';
    return true;
}
