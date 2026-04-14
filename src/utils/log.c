#include "log.h"
#include "drivers/framebuffer/framebuffer.h"
#include <stdarg.h>

#define FONT_WIDTH   8
#define FONT_HEIGHT  16
#define CHAR_SPACING 1
#define LINE_SPACING 1
#define LINE_HEIGHT  (FONT_HEIGHT + LINE_SPACING)

uint8_t log_level = 0; // 0-5
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

/**
 * @brief Writes a single character to the framebuffer.
 *
 * @param c Character to write
 * @param color Text color
 */
void put_char(char c, uint32_t color) {
    // new line parameter
    if (c == '\n')
    {
        cursor_y += (FONT_HEIGHT + LINE_SPACING);
        cursor_x = 0;
        return;
    }

    draw_char(c, cursor_y, cursor_x, color);
    cursor_x += FONT_WIDTH + CHAR_SPACING;

    // check if there is enoug space for new char
    if((active_fb->width - cursor_x) < FONT_WIDTH) {
        cursor_x = 0;
        cursor_y += FONT_HEIGHT + LINE_SPACING;
    }
}

/**
 * @brief Prints a string to the framebuffer with a given color.
 *
 * @param str Null-terminated string
 * @param color Text color
 */
void fb_print(const char *str, uint32_t color) {
    if (active_fb == NULL) return;
    if (active_fb->address == NULL) return;
    while (*str)
    {
        put_char(*str, color);
        str++;
    }
}

/**
 * @brief Clears the screen.
 */
void clear_screen() {
    fill(BLACK);
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * @brief Converts a decimal integer to string and appends it to buffer.
 *
 * @param buf Output buffer
 * @param size Size of the buffer
 * @param index Current write index (updated after writing)
 * @param value Integer value to convert
 */
void dec_to_str(char buf[256], size_t *buf_index, int dec) {
    if (dec == 0)
    {
        if (*buf_index < 255) buf[(*buf_index)++] = '0';
        return;
    }
    
    if (dec < 0)
    {
        if (*buf_index < 255) buf[(*buf_index)++] = '-';
        // Use unsigned arithmetic to avoid INT_MIN overflow
        uint32_t udec = (uint32_t)(-(int64_t)dec);
        size_t start = *buf_index;
        while (udec != 0)
        {
            int number = udec % 10;
            udec = udec / 10;
            if (*buf_index < 255) buf[(*buf_index)++] = '0' + number;
        }
        size_t end = *buf_index - 1;
        while (start < end)
        {
            uint8_t first_number = buf[start];
            uint8_t last_number = buf[end];
            buf[end] = first_number;
            buf[start] = last_number;
            start++;
            end--;
        }
        return;
    }

    size_t start = *buf_index;
    
    while (dec != 0)
    {
        int number = dec % 10;
        dec = dec / 10;
        if (*buf_index < 255) buf[(*buf_index)++] = '0' + number;
    }
    
    size_t end = *buf_index - 1;

    while (start < end)
    {
        uint8_t first_number = buf[start];
        uint8_t last_number = buf[end];
        buf[end] = first_number;
        buf[start] = last_number;
        start++;
        end--;
    }
}

/**
 * @brief Converts an unsigned integer to string and appends it to buffer.
 *
 * @param buf Output buffer
 * @param size Size of the buffer
 * @param index Current write index (updated after writing)
 * @param value Unsigned integer value to convert
 */

void uint_to_str(char buf[256], size_t *buf_index, uint64_t uint) {
    if (uint == 0)
    {
        if (*buf_index < 255) buf[(*buf_index)++] = '0';
        return;
    }

    size_t start = *buf_index;
    
    while (uint != 0)
    {
        int number = uint % 10;
        uint = uint / 10;
        if (*buf_index < 255) buf[(*buf_index)++] = '0' + number;
    }
    
    size_t end = *buf_index - 1;

    while (start < end)
    {
        uint8_t first_number = buf[start];
        uint8_t last_number = buf[end];
        buf[end] = first_number;
        buf[start] = last_number;
        start++;
        end--;
    }
}

/**
 * @brief Converts a hexadecimal value to string and appends it to buffer.
 *
 * @param buf Output buffer
 * @param size Size of the buffer
 * @param index Current write index (updated after writing)
 * @param value Hexadecimal value to convert
 */
void hex_to_str(char buf[256], size_t *buf_index, uint64_t hex) {
    if (*buf_index < 255) buf[(*buf_index)++] = '0';
    if (*buf_index < 255) buf[(*buf_index)++] = 'x';

    if (hex == 0x0)
    {
        if (*buf_index < 255) buf[(*buf_index)++] = '0';
        return;
    }
    

    char hex_table[] = "0123456789ABCDEF";

    size_t start = *buf_index;
    while (hex != 0x0)
    {
        uint8_t value = hex % 16;
        hex /= 16;
        if (*buf_index < 255) buf[(*buf_index)++] = hex_table[value];
    }
    size_t end = *buf_index - 1;

    while (start < end)
    {
        uint8_t first_number = buf[start];
        uint8_t last_number = buf[end];
        buf[end] = first_number;
        buf[start] = last_number;
        start++;
        end--;
    }
}

/**
 * @brief Writes a formatted string into a buffer using a va_list.
 *
 * @param buf Output buffer
 * @param size Size of the buffer
 * @param format Format string (printf-style)
 * @param args Argument list (va_list)
 * @return Number of characters written (excluding null terminator)
 */
void vsn_print(char buf[256], const char *str, va_list arg_list) {
    size_t index = 0;   // index
    int d;              // decimal
    char c;             // char
    char *s;            // string
    uint64_t x;         // hex
    uint64_t u;         // unsinged decimal

    while (*str)
    {
        if (*str == '%')
        {
            str++;
            switch (*str++)
            {        
            case 'c': // char
                c = (char)va_arg(arg_list, int);
                if (index < 255) buf[index++] = c;
                break;
                
            case 's': // string
                s = va_arg(arg_list, char*);
                while (*s)
                {
                    if (index < 255) buf[index++] = *s++;
                }
                break;
                
            case 'd': // decimal
                d = va_arg(arg_list, int);
                dec_to_str(buf, &index, d);
                break;
            case 'x': //  hex
                x = va_arg(arg_list, uint64_t);
                hex_to_str(buf, &index, x);
                break;
            
            case 'u': // unsinged int
                u = va_arg(arg_list, uint64_t);
                uint_to_str(buf, &index, u);
                break;
            
            default:
                if (index < 255) buf[index++] = '?';
                break;
            }
        } else {
            if (index < 255) buf[index++] = *str++;
        }
    }
    buf[index < 256 ? index : 255] = '\0';
}

/**
 * @brief Prints a formatted string with a given color using a va_list.
 *
 * @param format Format string (printf-style)
 * @param args Argument list (va_list)
 * @param color Text color
 */
void printf(const char *str, va_list arg_list, uint32_t color) {
    char buffer[256];
    vsn_print(buffer, str, arg_list);
    fb_print(buffer, color);
    // TODO: ADD UART PRINT
}

/**
 * @brief Logs a formatted message with a given log level.
 *
 * @param log_type Log level (LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG)
 * @param str Format string (printf-style)
 * @param ... Additional arguments for formatting
 */
void log(uint8_t log_type, const char* str, ...) {
    uint32_t color = WHITE;
    va_list arg_list;
    va_start(arg_list, str);

    //* log colors
    if (log_type == LOG_TYPE_ERROR) color = RED;
    if (log_type == LOG_TYPE_WARNING) color = YELLOW;
    if (log_type == LOG_TYPE_INFO) color = WHITE;
    if (log_type == LOG_TYPE_DEBUG) color = GREEN;

    //* print nothing
    if (log_level == LOG_LVL_SILENT)
    {
        va_end(arg_list);
        return;
    }

    //* print everything
    if (log_level == LOG_LVL_VERBOSE)
    {
        printf(str, arg_list, color);
        va_end(arg_list);
        return;
    }
    
    //* print only if message meets current log level
    //* -1 offsets the difference between log_level and log_type scales (LOG_LVL_VERBOSE has no log_type equivalent)
    if (log_type >= (log_level - 1))
    {
        printf(str, arg_list, color);
        va_end(arg_list);
        return;
    }

    va_end(arg_list);
}