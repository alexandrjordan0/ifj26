/**
 * File: precedence.h
 * Project: IFJ26
 * Authors: Alexandr Jordán (xjordaa00)
 * Translator: gcc 16.2.1
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "error.h"
#include "scanner.h"
// TODO #include "symtable.h"
#include <stdbool.h>

// TODO use symtable, after its done
typedef enum {
    EXPR_TYPE_NONE,
    EXPR_TYPE_INT,
    EXPR_TYPE_DOUBLE,
    EXPR_TYPE_STRING,
    EXPR_TYPE_BOOL
} ExprDataType;

// Symbols on the precedence stack
typedef enum {
    PREC_SYM_PLUS,   // +
    PREC_SYM_MINUS,  // -
    PREC_SYM_MUL,    // *
    PREC_SYM_DIV,    // /
    PREC_SYM_EQ,     // ==
    PREC_SYM_NEQ,    // !=
    PREC_SYM_LT,     // <
    PREC_SYM_GT,     // >
    PREC_SYM_LTE,    // <=
    PREC_SYM_GTE,    // >=
    PREC_SYM_NOT,    // ! (BOOLTHEN)
    PREC_SYM_AND,    // && (BOOLTHEN)
    PREC_SYM_OR,     // || (BOOLTHEN)
    PREC_SYM_LPAR,   // (
    PREC_SYM_RPAR,   // )
    PREC_SYM_ID,     // i (identifier or literal)
    PREC_SYM_DOLLAR, // $ (stack bottom or end of expression)
    PREC_SYM_STOP,   // < (stop marker for reduction handle)
    PREC_SYM_NONTERM // E (reduced nonterminal)
} PrecSymbol;

// Item on the precedence stack
typedef struct PrecStackItem {
    PrecSymbol symbol;
    ExprDataType data_type; // Type of nonterminal E for type checking and int2float
    struct PrecStackItem *next;
} PrecStackItem;

// The stack itself
typedef struct {
    PrecStackItem *top;
} PrecStack;

/**
 * @brief Main function for expression analysis and evaluation.
 * @param current_token Pointer to the current token on input first token of expression on output
 * first token AFTER expression.
 * @param symtable Pointer to the symbol table for variable and function verification.
 * @param result_type Output parameter where the function writes the resulting data type of the
 * expression.
 * @return ErrorCode 0 is OK 2 is syntactic error 3 to 7 are semantic or type errors 99 is internal.
 */
// TODO ErrorCode parse_expression(Token *current_token, symtable_t *symtable, ExprDataType
// *result_type);

#endif // PRECEDENCE_H
