/**
 * File: mem_manager.h
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H

#include <stdio.h>

// Structure of node
typedef struct mem_node_t {
    void *mem_prt;   // Pointer to the allocated memory
    size_t mem_size; // Size of the allocated memory in bytes
    struct mem_node_t *next;
    struct mem_node_t *prev;
} mem_node_t;

// Structure of double linked-list
typedef struct {
    mem_node_t *first;
    mem_node_t *last;
    mem_node_t *current;
    unsigned size;
} mem_list_t;

/**
 * @brief Initializes double linked-list
 * @param l Pointer to structure mem_list_t
 * @return void
 */
void memory_init(mem_list_t *l);

#endif // MEM_MANAGER_H
