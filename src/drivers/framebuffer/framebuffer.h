#pragma once

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

extern struct limine_framebuffer *active_fb;

enum basic_color : uint32_t {
    BLACK       = 0x000000,
    WHITE       = 0xFFFFFF,
    RED         = 0xFF0000,
    GREEN       = 0x00FF00,
    BLUE        = 0x0000FF, 
    YELLOW      = 0xFFFF00, 
    CYAN        = 0x00FFFF,
    MAGENTA     = 0xFF00FF,
    ORANGE      = 0xFFA500,
    PURPLE      = 0x800080,
    BROWN       = 0xA52A2A,
    GRAY        = 0x808080  
};

void framebuffer_init(struct limine_framebuffer_response *Framebuffer);
void draw_char(uint8_t c, size_t line, size_t column, uint32_t color);
void fill(uint32_t color);