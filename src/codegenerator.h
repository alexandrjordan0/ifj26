// File: codegenerator.h
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

// TODO #include "precedence.h"
#include "scanner.h"
#include "symtable.h"
#include <stdbool.h>
#include <stddef.h>

// @brief prepares input buffer and defines help GF variables
void gen_init(void);

/* @brief adds CALL on main and EXIT int@0, adds builtin functions
 *  and writes buffer on stdout
 * @param main_name
 */
void gen_finish(const char *main_name);

// @brief frees buffer
void gen_free(void);

// @brief return new unique number for code label
int gen_new_label_id(void);

/*
 * @brief PUSHS literals from token
 * @param token pointer to token to be pushed
 */
void gen_push_literal(const Token *token);

/* @brief PUSHSH LF@name$depth
 * @param name pinter to variable name
 * @param scope_depth depth of the scope
 */
void gen_push_var(const char *name, int scope_depth);

// @brief PUSHS nil@nil
void gen_push_unit(void);

/* @brief stack instruction for operator
 * @param op operation symbol
 * @param operand_type type of operand
 */
// TODO void gen_expr_op(PrecSymbol op, Type operand_type);

// @brief INT@FLOATS values on top
void gen_conv_top_int2float(void);

// @brief converts second value from top
void gen_conv_second_int2float(void);

/* @brief tests left operant for && or ||, when needed puts result to stack and jumps to end
 * @param op operation symbol
 * @param label_id id of label
 */
// TODO void gen_logic_start(PrecSymbol op, int label_id);

/* @brief code label of
 * @param
 * @param
 */
// TODO void gen_logic_end(PrecSymbol op, int label_id);

#endif
