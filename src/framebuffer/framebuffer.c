#include "framebuffer.h"

struct limine_framebuffer_response *framebuffer;
struct limine_framebuffer *active_fb = NULL;

static uint32_t make_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << active_fb->red_mask_shift)
         | ((uint32_t)g << active_fb->green_mask_shift)
         | ((uint32_t)b << active_fb->blue_mask_shift);
}

void set_pixel(uint32_t color, size_t x, size_t y){
    uint32_t *pixel = (uint32_t *)((uint8_t *)active_fb->address + y * active_fb->pitch + x * 4);
    *pixel = color;
}

// Test function
void fill(uint32_t color){
    for (size_t y = 0; y < 100 - 1; y++)
    {
        for (size_t x = 0; x < 100 - 1; x++)
        {
            set_pixel(color, x, y);
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

    uint32_t red = make_color(0xff, 0x00, 0x00);
    fill(red);
}