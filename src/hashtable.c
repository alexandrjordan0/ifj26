// File: hashtable.c
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#include "hashtable.h"
#include "error.h"
#include <stdlib.h>

typedef struct {
    char *key;
    void *value;
} HtableItem;

struct Htable {
    HtableItem **items;
    size_t capacity;
    size_t length;
};

Htable *hashtable_init(size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }
    Htable *hashtable = calloc(capacity, sizeof(Htable));
    if (hashtable == NULL) {
        error_exit(ERR_INTERNAL, "in hashtable.c allocation failure");
    }
    hashtable->capacity = capacity;
    hashtable->length = 0;
    hashtable->items = malloc(sizeof(HtableItem*) * capacity);
    if (hashtable->items == NULL) {
        free(hashtable);
        error_exit(ERR_INTERNAL, "in hashtable.c allocation failure");
    }
    return hashtable;
}

