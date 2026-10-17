/**
 * File: mem_manager.c
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#include "mem_manager.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

void mem_init(mem_list_t *l) {
    if (l == NULL) {
        return;
    }

    l->first = NULL;
    l->size = 0;
}

void *mem_alloc(mem_list_t *l, size_t size) {
    if (l == NULL) {
        return NULL;
    }

    mem_node_t *tmp_node = (mem_node_t *)malloc(sizeof(mem_node_t));
    if (tmp_node == NULL) {
        error_exit(ERR_INTERNAL, "Failed to allocate NEW ELEMENT (mem-manager)");
    }

    tmp_node->mem_ptr = (void *)malloc(size);
    if (tmp_node->mem_ptr == NULL) {
        free(tmp_node);
        error_exit(ERR_INTERNAL, "Failed allocation for MEMORY BLOCK (memory-manager)");
    }

    tmp_node->next = l->first;
    l->first = tmp_node;
    l->size++;
    tmp_node->mem_size = size;

    return tmp_node->mem_ptr;
}

void mem_free_all(mem_list_t *l) {
    if (l == NULL) {
        return;
    }

    mem_node_t *tmp_node;

    while (l->first != NULL) {
        tmp_node = l->first;
        l->first = tmp_node->next;

        free(tmp_node->mem_ptr);
        free(tmp_node);
    }

    l->size = 0;
}

unsigned mem_list_len(mem_list_t *l) {
    if (l == NULL) {
        return 0;
    }
    return (l->size);
}
