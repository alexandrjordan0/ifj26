/**
 * File: parser.h
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#ifndef PARSER_H
#define PARSER_H

#include "error.h"
#include "scanner.h"
#include "str.h"
#include <stdio.h>

// For simulating derivation tree
typedef struct tree_node_t {
    Token token_t;
    Token_value token_value_t;
    struct tree_node_t *left;
    struct tree_node_t *right;
} tree_node_t;

/**
 * @brief Initiate derivation tree
 * @param void
 * @return void
 */
void tree_init(void);
#endif
