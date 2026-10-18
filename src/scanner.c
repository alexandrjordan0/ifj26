// File: scanner.c
// Project: IFJ26
// Authors: Patrik Lisál (xlisalp00)

#include "scanner.h"
#include "str.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int line = 1;
static int col = 0;
static bool at_line_start = true;

#define INDENT_STACK_MAX 256
static int indent_top = 0; // index of the top of the stack
static int indent_stack[INDENT_STACK_MAX] = {0};
static int pending_dedents = 0; // amount of DEDENT tokens to be emited
static bool pending_indent = false;

/*------------------------*/
/* STATES OF AUTOMATA     */
/*------------------------*/

typedef enum {
    S_START,

    S_INDENT,

    S_ZERO, // "0" at beginning of int
    S_INT,
    S_INT_UNDERSCORE,
    S_INT_DOT,
    S_DOUBLE,
    S_EXP,
    S_EXP_SIGN,
    S_EXP_DIGITS,

    S_QUOTE1, // saw "
    S_QUOTE2, // saw ""
    S_STRING,
    S_STRING_ESC,
    S_MLSTRING,

    S_SLASH,
    S_LINE_COMMENT,
    S_BLOCK_COMMENT,

    S_LT,
    S_GT,
    S_ASSIGN,
    S_NOT,
    S_AND,
    S_OR,
    S_LPAR
} State;

/*------------------------*/
/* KEYWORD TABLE          */
/*------------------------*/

static const struct {
    const char *name;
    Token_type type;
} keywords[] = {
    {"def", TOKEN_KEYWORD_DEF},
    {"do", TOKEN_KEYWORD_DO},
    {"Double", TOKEN_KEYWORD_DOUBLE},
    {"if", TOKEN_KEYWORD_IF},
    {"else", TOKEN_KEYWORD_ELSE},
    {"import", TOKEN_KEYWORD_IMPORT},
    {"Int", TOKEN_KEYWORD_INT},
    {"return", TOKEN_KEYWORD_RETURN},
    {"String", TOKEN_KEYWORD_STRING},
    {"then", TOKEN_KEYWORD_THEN},
    {"Unit", TOKEN_KEYWORD_UNIT},
    {"val", TOKEN_KEYWORD_VAL},
    {"var", TOKEN_KEYWORD_VAR},
    {"while", TOKEN_KEYWORD_WHILE},
    // BOOLTHEN
    {"Boolean", TOKEN_KEYWORD_BOOLEAN},
    {"true", TOKEN_KEYWORD_TRUE},
    {"false", TOKEN_KEYWORD_FALSE},
};

/*------------------------*/
/* MAIN FUNCTION          */
/*------------------------*/
ErrorCode get_next_token(Token *token) {
    ErrorCode err;
    State state = S_START;
    string_t buf;
    int quotes = 0;
    int depth = 0;

    memset(token, 0, sizeof *token);
    token->line = line;
    token->col = col;

    // indentaion at the beginning of a line
    if (at_line_start) {
        at_line_start = false;
        /*TODO
         err = handle_indentation()
         if (err != ERR_OK) {
            return err;
        }*/
    }

    // waiting indent/dedent
    if (pending_dedents > 0) {
        pending_dedents--;
        token->type = TOKEN_DEDENT;
        if (err != ERR_OK) {
            return err;
        }
    }
    if (pending_indent) {
        pending_indent = false;
        token->type = TOKEN_INDENT;
        return ERR_OK;
    }

    if (!str_init(&buf)) {
        return ERR_INTERNAL;
    }
}