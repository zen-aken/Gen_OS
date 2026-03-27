#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uint8_t kstatus;
#define ksuccess 0
#define kfail 1

//? log levels
#define LOG_LVL_VERBOSE 0
#define LOG_LVL_DEBUG 1
#define LOG_LVL_INFO 2
#define LOG_LVL_WARNING 3
#define LOG_LVL_ERROR 4
#define LOG_LVL_SILENT 5

//? log types
#define LOG_TYPE_DEBUG   0
#define LOG_TYPE_INFO    1
#define LOG_TYPE_WARNING 2
#define LOG_TYPE_ERROR   3

void clear_screen();
void print(const char *str, ...);
void log(uint8_t log_level, const char* str, ...);