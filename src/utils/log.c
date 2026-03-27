#include "log.h"
#include "drivers/framebuffer/framebuffer.h"
#include <stdarg.h>

#define FONT_WIDTH   8
#define FONT_HEIGHT  16
#define CHAR_SPACING 1
#define LINE_SPACING 1
#define LINE_HEIGHT  (FONT_HEIGHT + LINE_SPACING)

uint8_t log_level = 0;
uint32_t cursor_x = 0;
uint32_t cursor_y = 0;

//* print a char to screen
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

//* framebuffer print
void fb_print(const char *str, uint32_t color) {
    while (*str)
    {
        put_char(*str, color);
        str++;
    }
}

//* clears screen
void clear_screen() {
    fill(BLACK);
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * Converts a decimal integer to string and writes it into buf at buf_index.
 * Digits are written in reverse order then flipped in place.
 * @param buf       output buffer
 * @param buf_index current write position, updated after writing
 * @param dec       integer to convert
 */
void dec_to_str(char buf[256], size_t *buf_index, int dec) {
    if (dec == 0)
    {
        buf[(*buf_index)++] = '0';
        return;
    }
    
    if (dec < 0)
    {
        buf[(*buf_index)++] = '-';
        dec = dec * (-1);
    }

    size_t start = *buf_index;
    
    while (dec != 0)
    {
        int number = dec % 10;
        dec = dec / 10;
        buf[(*buf_index)++] = '0' + number;
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
 * 
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
            case 'd': // decimal
                d = va_arg(arg_list, int);
                dec_to_str(buf, &index, d);
                break;
            
            case 'c': // char
                c = va_arg(arg_list, char);
                buf[index++] = c;
                break;
            
            case 's': // string
                s = va_arg(arg_list, char*);
                while (*s)
                {
                    buf[index++] = *s++;
                }
                break;
            
            case 'x': //  hex
                x = va_arg(arg_list, uint64_t);
                break;
            
            case 'u': // unsinged int
                x = va_arg(arg_list, uint64_t);
                break;
            
            default:
                buf[index++] = *str;
                break;
            }
        } else {
            buf[index++] = *str++;
        }
    }
    buf[index++] = '\0';
}

void print(const char *str, ...) {
    va_list arg_list;
    va_start(arg_list, str);
    char buffer[256];
    vsn_print(buffer, str, arg_list);
    fb_print(buffer, WHITE);
    va_end(arg_list);
}

/**
 * @param log_level (LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG)
 * @param str log message
 */
void log(uint8_t log_type, const char* str, ...) {
    // TODO: ADD FORMAT SUPPORT
    uint32_t color;
    if (log_type == LOG_TYPE_ERROR) color = RED;
    if (log_type == LOG_TYPE_WARNING) color = YELLOW;
    if (log_type == LOG_TYPE_INFO) color = WHITE;
    if (log_type == LOG_TYPE_DEBUG) color = GREEN;

    if (log_level == LOG_LVL_SILENT)
    {
        return;
    }
    
    if (log_level == LOG_LVL_VERBOSE)
    {
        fb_print(str, color);
        // TODO: ADD UART
        return;
    }
    
    //* print only if message meets current log level
    //* -1 offsets the difference between log_level and log_type scales (LOG_LVL_VERBOSE has no log_type equivalent)
    if (log_type >= (log_level - 1))
    {
        fb_print(str, color);
        // TODO: ADD UART
        return;
    }
}