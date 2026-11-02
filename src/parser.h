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

#if 0
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

ErrorCode parse_program();

ErrorCode parse_prolog();

ErrorCode parse_func_set();

ErrorCode parse_func();

ErrorCode parse_main_anot();

ErrorCode parse_main_anot_eol();

ErrorCode parse_param_set();

ErrorCode parse_param_next();

ErrorCode parse_param();

ErrorCode parse_type();

ErrorCode parse_return_type();

ErrorCode parse_body();

ErrorCode parse_variable();

ErrorCode parse_variable_type();

ErrorCode parse_value();

ErrorCode parse_value_type();

ErrorCode parse_block();

ErrorCode parse_command_set();

ErrorCode parse_command();

ErrorCode parse_command_id();

ErrorCode parse_func_call();

ErrorCode parse_term();

ErrorCode parse_arg_set();

ErrorCode parse_arg_next();

ErrorCode parse_if_statement();

ErrorCode parse_if_body();

ErrorCode parse_if_block();

ErrorCode parse_else_block();

ErrorCode parse_whlile_statement();

ErrorCode parse_while_body();

ErrorCode parse_while_block();

ErrorCode parse_volsemcol();

ErrorCode parse_return_command();

#endif
