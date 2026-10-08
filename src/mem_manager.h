/**
 * File: mem_manager.h
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H

#include <stdio.h>
#include <stdlib.h>

// Structure of node
typedef struct mem_node_t {
    void *mem_ptr;   // Pointer to the allocated memory
    size_t mem_size; // Size of the allocated memory in bytes
    struct mem_node_t *next;
} mem_node_t;

// Structure of single linked-list
typedef struct {
    mem_node_t *first; // Pointer to the first element in list
    unsigned size;     // Number of elements in list
} mem_list_t;

/**
 * @brief Initializes linked-list
 * @param l Pointer to structure mem_list_t
 * @return void
 */
void memory_init(mem_list_t *l);

/**
 * @brief Allocates block of memory and adds this element to list
 * @param l Pointer to structure mem_list_t
 * @param size is a number of bytes to allocate
 * @return generic void Pointer
 */
void *mem_alloc(mem_list_t *l, size_t size);

/**
 * @brief Frees all allocated blocks of memory
 * @param l Pointer to structure mem_list_t
 * @return void
 */
void mem_free_all(mem_list_t *l);

/**
 * @brief Retruns number of elements in list
 * @param l Pointer to structurer mem_list_t
 * @return unsigned integer
 */
unsigned mem_list_len(mem_list_t *l);
#endif // MEM_MANAGER_H
