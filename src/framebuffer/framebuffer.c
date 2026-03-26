#include "framebuffer.h"
#include "fonts/font_8x16.h"
#include <log.h>

#define FONT8x16_IMPLEMENTATION

struct limine_framebuffer_response *framebuffer;
struct limine_framebuffer *active_fb = NULL;

//! UNUSED FOR NOW
// static uint32_t make_color(uint8_t r, uint8_t g, uint8_t b) {
//     return ((uint32_t)r << active_fb->red_mask_shift)
//          | ((uint32_t)g << active_fb->green_mask_shift)
//          | ((uint32_t)b << active_fb->blue_mask_shift);
// }

void set_pixel(uint32_t color, size_t x, size_t y){
    uint32_t *pixel = (uint32_t *)((uint8_t *)active_fb->address + y * active_fb->pitch + x * 4);
    *pixel = color;
}

// fill whole screen with one color
void fill(uint32_t color) {
    for (size_t y = 0; y < active_fb->height - 1; y++)
    {
        for (size_t x = 0; x < active_fb->width - 1; x++)
        {
            set_pixel(color, x, y);
        }
    }
}

/**
 * @param c is a char
 * @param x is pixel x (column)
 * @param y is pixel y (line/row)
 * @param color it's just a color (give '0' for white)
 */
void draw_char(uint8_t c, size_t line, size_t column, uint32_t color){
    if (color == 0) color = WHITE;
    uint8_t (*character)[16] = &font8x16[c];
    for (size_t row = 0; row < 16; row++)
    {
        for (size_t col = 0; col < 8; col++)
        {
            if ((*character)[row] & (1 << (7 - col)))
            {
                set_pixel(color, (col + column), (row + line));
            }
        }
    }
}

void framebuffer_init(struct limine_framebuffer_response *Framebuffer){
    //* NULL check
    if (Framebuffer == NULL || Framebuffer->framebuffer_count < 1)
    {
        return;
    }

    //* Set framebuffer
    framebuffer = Framebuffer;

    //* Set first display for terminal output
    active_fb = Framebuffer->framebuffers[0];
    log(LOG_INFO, "Framebuffer init success");
}
