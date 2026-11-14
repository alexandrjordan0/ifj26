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
#include "symtable.h"
#include <stdio.h>

ErrorCode parse_program();

ErrorCode parse_prolog();

ErrorCode parse_func_set();

ErrorCode parse_func();

ErrorCode parse_main_anot();

ErrorCode parse_main_anot_eol();

ErrorCode parse_param_set();

ErrorCode parse_param_next();

ErrorCode parse_param();

ErrorCode parse_type(Type *type);

ErrorCode parse_return_type();

ErrorCode parse_body();

ErrorCode parse_variable();

ErrorCode parse_variable_type();

ErrorCode parse_value();

ErrorCode parse_value_type();

ErrorCode parse_block();

ErrorCode parse_command_set();

ErrorCode parse_command();

ErrorCode parse_command_simple_id();

ErrorCode parse_command_simple();

ErrorCode parse_command_complex();

ErrorCode parse_arg_set();

ErrorCode parse_arg_next();

ErrorCode parse_term();

ErrorCode parse_if_statement();

ErrorCode parse_vol_else_block();

ErrorCode parse_else_block();

ErrorCode parse_while_statement();

ErrorCode parse_separator();

ErrorCode parse_return_command();

#endif
