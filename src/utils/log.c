#include "log.h"
#include "drivers/framebuffer/framebuffer.h"

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
void clear_screen(){
    fill(BLACK);
    cursor_x = 0;
    cursor_y = 0;
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