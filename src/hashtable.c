// File: hashtable.c
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#include "hashtable.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *key;
    void *value;
} HtableItem;

struct Htable {
    HtableItem **items;
    size_t capacity;
    size_t count;
};

// modified hash function from my IJC project
size_t hash(const char *key) {
    size_t h = 0;
    const unsigned char *p;
    for (p = (const unsigned char *)key; *p != '\0'; p++)
        h = 65599 * h + *p;
    return h;
}

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

// linear placement of items, possible upgrade to quadratic or double hash
static void place_item(HtableItem **items, size_t capacity, HtableItem *item) {
    size_t start = hash(item->key) % capacity;
    for (size_t i = 0; i < capacity; i++) {
        size_t pos = (i + start) % capacity;
        if (items[pos] == NULL) {
            items[pos] = item;
            return;
        }
    }
}

bool hashtable_check_size(Htable *hashtable) {
    if (hashtable->capacity * 7 >= hashtable->count * 10) {
        return true;
    }
    HtableItem **new_items = calloc(hashtable->capacity * 2, sizeof(HtableItem *));
    if (new_items == NULL) {
        return false;
    }
    for (size_t i = 0; i < hashtable->capacity; i++) {
        if (hashtable->items[i] != NULL) {
            place_item(new_items, hashtable->capacity * 2, hashtable->items[i]);
        }
    }
    free(hashtable->items);
    hashtable->capacity = hashtable->capacity * 2;
    hashtable->items = new_items;
    return true;
}

void *hashtable_search(Htable *hashtable, const char *key) {
    size_t pos = hash(key) % hashtable->capacity;
    for (size_t i = 0; i < hashtable->capacity; i++) {
        if (hashtable->items[(pos + i) % hashtable->capacity] == NULL) {
            return NULL;
        }
        if (strcmp(key, hashtable->items[(pos + i) % hashtable->capacity]->key) == 0) {
            return hashtable->items[(pos + i) % hashtable->capacity]->value;
        }
    }
    return NULL;
}