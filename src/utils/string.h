#pragma once

#include <stddef.h>

/**
 * @brief Compare two strings.
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief Get string length.
 */
size_t strlen(const char *s);

/**
 * @brief Copy string (including null terminator).
 * @return Destination pointer.
 */
char *strcpy(char *dest, const char *src);

/**
 * @brief Copy up to n characters.
 */
char *strncpy(char *dest, const char *src, size_t n);

/**
 * @brief Set memory to value.
 */
void *memset(void *s, int c, size_t n);

/**
 * @brief Copy memory (non-overlapping).
 */
void *memcpy(void *dest, const void *src, size_t n);

/**
 * @brief Move memory (handles overlap).
 */
void *memmove(void *dest, const void *src, size_t n);
