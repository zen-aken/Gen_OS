#include "log.h"
#include "framebuffer/framebuffer.h"

#define FONT_WIDTH   8
#define FONT_HEIGHT  16
#define CHAR_SPACING 1
#define LINE_SPACING 1
#define LINE_HEIGHT  (FONT_HEIGHT + LINE_SPACING)

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
void fb_print(char *str, uint32_t color) {
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

void log(uint8_t log_level, const char* str, ...) {
    //! ADD FORMAT SUPPORT
    uint32_t color;
    if (log_level == LOG_ERROR) color = RED;
    if (log_level == LOG_WARNING) color = YELLOW;
    if (log_level == LOG_INFO) color = WHITE;
    if (log_level == LOG_DEBUG) color = WHITE;
    
    //* print channel
    fb_print(str, color);
    //! ADD UART
}