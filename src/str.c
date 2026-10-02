// File: str.c
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#ifndef STR_H
#define STR_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Structure of dynamic string
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} string_t;

/**
 * @brief Inicializes dynamic string
 * @param s Pointer to structure string_t
 * @return true when succeeded, false when failure
 */
bool str_init(string_t *s);

/**
 * @brief Frees alocated memmory of string
 * @param s Pointer to structure string_t
 */
void str_free(string_t *s);

/**
 * @brief Clears dynamic string
 * @param s Pointer to structure string_t
 */
void str_clear(string_t *s);

/**
 * @brief Appends char the end of dynamic string
 * @param s Pointer to structure string_t
 * @param c Pointer to char
 * @return true when succeeded, false when failure
 */
bool str_append_char(string_t *s, char *c);

/**
 * @brief Appends C-string to the end of dynamic string
 * @param s Pointer to structure string_t
 * @param src Pointer to srouce string
 * @return true when succeeded, false when failure
 */
bool str_append_str(string_t *s, string_t *src);

#endif
