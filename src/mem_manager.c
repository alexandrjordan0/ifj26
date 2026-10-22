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

    mem_node_t *tmp_node = malloc(sizeof(mem_node_t));
    if (tmp_node == NULL) {
        mem_free_all(l);
        error_exit(ERR_INTERNAL, "Failed to allocate NEW ELEMENT (mem-manager)");
    }

    tmp_node->mem_ptr = malloc(size);
    if (tmp_node->mem_ptr == NULL) {
        free(tmp_node);
        mem_free_all(l);
        error_exit(ERR_INTERNAL, "Failed allocation for MEMORY BLOCK (mem-manager)");
    }

    tmp_node->next = l->first;
    l->first = tmp_node;
    l->size++;
    tmp_node->mem_size = size;

    return tmp_node->mem_ptr;
}

void *mem_realloc(mem_list_t *l, void *ptr, size_t new_size) {
    if (l == NULL) {
        return NULL;
    }

    void *tmp_ptr;
    if (ptr != NULL) {
        tmp_ptr = realloc(ptr, new_size);
        if (tmp_ptr == NULL) {
            mem_free_all(l);
            error_exit(ERR_INTERNAL, "Failed reallocation new MEMORY BLOCK (mem-manager)");
        }

        mem_node_t *tmp_node = l->first;
        while (tmp_node != NULL) {
            if (tmp_node->mem_ptr == ptr) {
                tmp_node->mem_ptr = tmp_ptr;
                tmp_node->mem_size = new_size;
                break;
            }
            tmp_node = tmp_node->next;
        }
    } else {
        tmp_ptr = mem_alloc(l, new_size);
    }

    return tmp_ptr;
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
