// File: str.h
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#ifndef STR_H
#define STR_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Structure of dynamic string
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} string_t;

/**
 * @brief Initializes dynamic string
 * @param s Pointer to structure string_t
 * @return true when succeeded, false when failure
 */
bool str_init(string_t *s);

/**
 * @brief Frees allocated memory of string
 * @param s Pointer to structure string_t
 */
void str_free(string_t *s);

/**
 * @brief Clears dynamic string
 * @param s Pointer to structure string_t
 */
void str_clear(string_t *s);

/**
 * @brief Appends a char to the end of dynamic string
 * @param s Pointer to structure string_t
 * @param c Char to append
 * @return true when succeeded, false when failure
 */
bool str_append_char(string_t *s, char c);

/**
 * @brief Appends C-string to the end of dynamic string
 * @param s Pointer to structure string_t
 * @param src Pointer to source string
 * @return true when succeeded, false when failure
 */
bool str_append_str(string_t *s, const char *src);

#endif // STR_H
