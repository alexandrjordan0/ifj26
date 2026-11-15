// File: codegenerator.h
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "precedence.h"
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

/* @brief PUSHS LF@name$depth
 * @param name pointer to variable name
 * @param scope_depth depth of the scope
 */
void gen_push_var(const char *name, int scope_depth);

// @brief PUSHS nil@nil
void gen_push_unit(void);

/* @brief stack instruction for operator
 * @param op operation symbol
 * @param operand_type type of operand
 */
void gen_expr_op(PrecSymbol op, Type operand_type);

// @brief INT2FLOATS value on top
void gen_conv_top_int2float(void);

// @brief converts second value from top of stack from int to float, top stays on top
void gen_conv_second_int2float(void);

/* @brief tests left operand for && or ||, when result is known right operand is skipped
 * @param op operation symbol
 * @param label_id id of label
 */
void gen_logic_start(PrecSymbol op, int label_id);

/* @brief code label of end of && or ||, right operand result stays on stack
 * @param op operation symbol
 * @param label_id id of label
 */
void gen_logic_end(PrecSymbol op, int label_id);

/* @brief DEFVAR LF@name$depth, inside while loop must be emitted only once
 * @param name pointer to variable name
 * @param scope_depth depth of the scope
 */
void gen_defvar(const char *name, int scope_depth);

/* @brief POPS LF@name$depth, value of expression must be on top of stack
 * @param name pointer to variable name
 * @param scope_depth depth of the scope
 */
void gen_assign(const char *name, int scope_depth);

// @brief POPS value on top to help GF variable
void gen_discard_top(void);

/* @brief LABEL name, CREATEFRAME, PUSHFRAME, DEFVAR params and POPS them from stack
 *  in reverse order
 * @param name pointer to function name
 * @param param_count number of parameters
 * @param param_names array of parameter names, length param_count
 * @param scope_depth depth of the scope of parameters
 * @param returns_unit true if function returns Unit (explicit or inferred), remembered for
 *  gen_func_end
 */
void gen_func_start(const char *name, size_t param_count, const char *const *param_names,
                    int scope_depth, bool returns_unit);

// @brief end of function, PUSHS nil@nil when returns Unit, POPFRAME and RETURN
void gen_func_end(void);

// @brief return value must be on top of stack (gen_push_unit for bare return), POPFRAME and RETURN
void gen_return(void);

/* @brief CALL name, arguments must be on stack, return value is on top of stack after call
 * @param name pointer to function name
 */
void gen_call(const char *name);

/* @brief condition must be on top of stack, PUSHS bool@false and JUMPIFEQS $else$id
 * @param label_id id of label
 */
void gen_if_start(int label_id);

/* @brief JUMP $endif$id and LABEL $else$id, must be called even without else branch
 * @param label_id id of label
 */
void gen_if_else(int label_id);

/* @brief LABEL $endif$id
 * @param label_id id of label
 */
void gen_if_end(int label_id);

/* @brief LABEL $while$id, called before generating condition
 * @param label_id id of label
 */
void gen_while_header(int label_id);

/* @brief condition must be on top of stack, PUSHS bool@false and JUMPIFEQS $endwhile$id
 * @param label_id id of label
 */
void gen_while_cond(int label_id);

/* @brief JUMP $while$id and LABEL $endwhile$id
 * @param label_id id of label
 */
void gen_while_end(int label_id);

#endif
