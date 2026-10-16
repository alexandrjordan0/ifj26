/**
 * File: parser.h
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#ifndef PARSER_H
#define PARSER_H

#include "error.h"
#include "str.h"
#include <stdio.h>

typedef struct tree_node_t {
    string_t token_t;
    string_t token_type_t;
    struct tree_node_t *left;
    struct tree_node_t *right;
} tree_node_t;

#endif
