/**
 * File: parser.c
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#include "parser.h"
#include "error.h"
#include "scanner.h"

Token token;

#define GET_TOKEN()                                                                                \
    do {                                                                                           \
        if (get_next_token(&token) != ERR_OK) {                                                    \
            return ERR_LEXICAL;                                                                    \
        }                                                                                          \
    } while (0)

ErrorCode parse_program() {
    if (parse_prolog() != ERR_OK) {
        return ERR_SYNTAX;
    }
    if (parse_func_set() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    if (token.type != TOKEN_EOF) {
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

ErrorCode parse_prolog() {
    GET_TOKEN();

    if (token.type != TOKEN_KEYWORD_IMPORT) {
        return ERR_SYNTAX;
    }
    if (token.type != TOKEN_IDENTIFIER) {
        return ERR_SYNTAX;
    }
    if (token.type != TOKEN_DOT) {
        return ERR_SYNTAX;
    }
    if (token.type != TOKEN_MUL) {
        return ERR_SYNTAX;
    }
    if (token.type != TOKEN_EOL) {
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

ErrorCode parse_func_set() {
    /**
     * TODO
     */
    return ERR_OK;
}
