// File: scanner.h
// Project: IFJ26
// Authors: Patrik Lisál (xlisalp00)

#ifndef SCANNER_H
#define SCANNER_H

#include "error.h"
#include "str.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_EOL,

    TOKEN_IDENTIFIER,

    TOKEN_KEYWORD_DEF,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_DOUBLE,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_IMPORT,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_STRING,
    TOKEN_KEYWORD_THEN,
    TOKEN_KEYWORD_UNIT,
    TOKEN_KEYWORD_VAL,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_WHILE,

    // BOOLTHEN
    TOKEN_KEYWORD_BOOLEAN,
    TOKEN_KEYWORD_TRUE,
    TOKEN_KEYWORD_FALSE,

    TOKEN_LIT_INT,
    TOKEN_LIT_DOUBLE,
    TOKEN_LIT_STRING,
    TOKEN_LIT_UNIT, // ()

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,

    TOKEN_EQ,  // ==
    TOKEN_NEQ, // !=
    TOKEN_LESSTHAN,
    TOKEN_GREATERTHAN,
    TOKEN_LESSOREQ,    // <=
    TOKEN_GREATEROREQ, // >=

    TOKEN_NOT, // !
    TOKEN_AND, // &&
    TOKEN_OR,  // ||

    TOKEN_ASSIGN, // =
    TOKEN_LEFTPAR,
    TOKEN_RIGHTPAR,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_AT,        // @
    TOKEN_DOT,       // .
    TOKEN_UNDERSCORE // _
} Token_type;

typedef union {
    long long int_value;
    double double_value;
    string_t str_value;
} Token_value;

typedef struct {
    Token_type type;
    Token_value value;
    int line;
    int col;
} Token;

/**
 * @brief reads next token
 * @param token pointer to Token
 * @return ERR_OK(0), ERR_LEXICAL(1), ERR_INTERNAL(99)
 */
ErrorCode get_next_token(Token *token);

/**
 * @brief frees allocated memory of token
 * @param token pointer to Token
 */
void token_free(Token *token);

#endif