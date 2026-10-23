// File: hashtable.c
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#include "hashtable.h"
#include <stdlib.h>

typedef struct {
    char *key;
    void *value;
} HtableItem;

struct Htable {
    HtableItem **items;
    size_t capacity;
    size_t count;
};

Htable *hashtable_init(size_t capacity) {
    if (capacity == 0) {
        return NULL;
    }
    Htable *hashtable = malloc(sizeof(Htable));
    if (hashtable == NULL) {
       return NULL;
    }
    hashtable->capacity = capacity;
    hashtable->count = 0;
    hashtable->items = calloc(capacity, sizeof(HtableItem *));
    if (hashtable->items == NULL) {
        free(hashtable);
        return NULL;
    }
    return hashtable;
}

void hashtable_delete(Htable *hashtable) {
    if (hashtable == NULL) {
        return;
    }
    for (size_t i = 0; i < hashtable->capacity; i++) {
        if (hashtable->items[i] != NULL) {
            free(hashtable->items[i]->key);
            free(hashtable->items[i]);
        }
    }
    free(hashtable->items);
    free(hashtable);
}


