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
    size_t count;
};