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

    S_IDENT,

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

    for (;;) {
        // int c = look_next_char();

        switch (state) {
        case S_START:
            break;

        case S_IDENT:
            break;

        case S_ZERO:
            break;

        case S_INT:
            break;

        case S_INT_UNDERSCORE:
            break;

        case S_INT_DOT:
            break;

        case S_DOUBLE:
            break;

        case S_EXP:
            break;

        case S_EXP_SIGN:
            break;

        case S_EXP_DIGITS:
            break;

        case S_QUOTE1:
            break;

        case S_QUOTE2:
            break;

        case S_STRING:
            break;

        case S_STRING_ESC:
            break;

        case S_MLSTRING:
            break;

        case S_SLASH:
            break;

        case S_LINE_COMMENT:
            break;

        case S_BLOCK_COMMENT:
            break;

        case S_LT:
            break;

        case S_GT:
            break;

        case S_ASSIGN:
            break;

        case S_NOT:
            break;

        case S_AND:
            break;

        case S_OR:
            break;

        case S_LPAR:
            break;
        }
    }
}

void token_free(Token *token) {
    if (token == NULL) {
        return;
    }
    if (token->type == TOKEN_IDENTIFIER || token->type == TOKEN_LIT_STRING) {
        str_free(&token->value.str_value);
    }
}