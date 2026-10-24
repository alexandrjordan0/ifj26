// File: hashtable.h
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Htable Htable;

/**
 * @brief create new hashtable
 * @param capacity of hashtable
 * @return pointer to created hashtable, NULL if capacity == 0 or allocation failure
 */
Htable *hashtable_init(size_t capacity);

/**
 * @brief delete hashtable and keys, does not delete value
 * @param hashtable pointer to hashtable
 */
void hashtable_delete(Htable *hashtable);

/**
 * @brief insert new item to hashtable, key is copied and hashtable owns it. The value must stay
 * valid while the table exists.
 * @param hashtable pointer to hashtable
 * @param value Pointer to added value, cant be NULL
 * @param key Pointer to string
 * @return true if OK, false if key already exists
 */
bool hashtable_add(Htable *hashtable, void *value, const char *key);

/**
 * @brief search for value of given key, value cant be NULL
 * @param hashtable pointer to hashtable
 * @param key Pointer to string
 * @return pointer on value, NULL if key does not exist
 */
void *hashtable_search(Htable *hashtable, const char *key);

#endif