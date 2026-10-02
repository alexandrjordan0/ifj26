// File: str.c
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#include "str.h"

#define STR_INIT_CAPACITY 16

bool str_init(string_t *s) {
    if (s == NULL) {
        return false;
    }

    s->data = malloc(STR_INIT_CAPACITY * sizeof(char));
    if (s->data == NULL) {
        return false;
    }

    s->data[0] = '\0';
    s->length = 0;
    s->capacity = STR_INIT_CAPACITY;

    return true;
}

void str_free(string_t *s) {
    if (s == NULL) {
        return;
    }

    if (s->data != NULL) {
        free(s->data);
        s->data = NULL;
    }

    s->length = 0;
    s->capacity = 0;
}

void str_clear(string_t *s) {
    if (s == NULL) {
        return;
    }

    s->length = 0;
    if (s->data != NULL) {
        s->data[0] = '\0';
    }
}

bool str_append_char(string_t *s, char c) {
    if (s == NULL || s->data == NULL) {
        return false;
    }

    if (s->length + 1 >= s->capacity) {
        size_t new_capacity = (s->capacity == 0) ? STR_INIT_CAPACITY : s->capacity * 2;
        char *new_data = realloc(s->data, new_capacity * sizeof(char));
        if (new_data == NULL) {
            return false;
        }

        s->data = new_data;
        s->capacity = new_capacity;
    }

    s->data[s->length] = c;
    s->length++;
    s->data[s->length] = '\0';

    return true;
}

bool str_append_str(string_t *s, const char *src) {
    if (s == NULL || s->data == NULL || src == NULL) {
        return false;
    }

    size_t src_len = strlen(src);
    if (src_len == 0) {
        return true;
    }

    size_t required_capacity = (s->length + src_len + 1);
    if (required_capacity > s->capacity) {
        size_t new_capacity = (s->capacity == 0) ? STR_INIT_CAPACITY : s->capacity * 2;
        if (new_capacity < required_capacity) {
            new_capacity = required_capacity;
        }

        char *new_data = realloc(s->data, new_capacity * sizeof(char));
        if (new_data == NULL) {
            return false;
        }

        s->data = new_data;
        s->capacity = new_capacity;
    }

    memcpy(s->data + s->length, src, src_len);
    s->length += src_len;
    s->data[s->length] = '\0';

    return true;
}
