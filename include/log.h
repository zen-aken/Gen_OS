#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t kstatus;
#define ksuccess 0
#define kfail 1

#define LOG_ERROR   0
#define LOG_WARNING 1
#define LOG_INFO    2
#define LOG_DEBUG   3

void clear_screen();
void fb_print(char *str, uint32_t color);
void log(uint8_t log_level, const char* str, ...);