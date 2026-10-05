/**
 * File: mem_manager.c
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#include "mem_manager.h"

void memory_init(mem_list_t *l) {
    l->first = NULL;
    l->last = NULL;
    l->current = NULL;
    l->size = 0;
}
