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

int parse_program();

int parse_prolog();

int parse_func_set();

int parse_func();

int parse_main_anot();

int parse_param_set();

int parse_param_next();

int parse_param();

int parse_type();

int parse_return_type();

int parse_body();

int parse_variable();

int parse_variable_type();

int parse_value();

int parse_value_type();

int parse_block();

int parse_command_set();

int parse_command();

int parse_command_id();

int parse_func_call();

int parse_term();

int parse_arg_set();

int parse_arg_next();

int parse_if_statement();

int parse_if_body();

int parse_if_block();

int parse_else_block();

int parse_whlile_statement();

int parse_while_body();

int parse_while_block();
#endif
