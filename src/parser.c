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
        token_free(&token);                                                                        \
        if (get_next_token(&token) != ERR_OK) {                                                    \
            return ERR_LEXICAL;                                                                    \
        }                                                                                          \
    } while (0)

#define CHECK_TOKEN(something)                                                                     \
    do {                                                                                           \
        if (token.type != something) {                                                             \
            return ERR_SYNTAX;                                                                     \
        }                                                                                          \
    } while (0)

ErrorCode parse_program() {

    /* Symtable init */

    if (parse_prolog() != ERR_OK) {
        return ERR_SYNTAX;
    }
    if (parse_func_set() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOF);

    return ERR_OK;
}

ErrorCode parse_prolog() {
    GET_TOKEN();
    CHECK_TOKEN(TOKEN_KEYWORD_IMPORT);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_DOT);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_MUL);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOL);
    return ERR_OK;
}

ErrorCode parse_func_set() {
    if (parse_func() != ERR_OK) {
        return ERR_SYNTAX;
    }

    if (parse_func_set() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK;
}

ErrorCode parse_func() {
    if (parse_main_anot() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURRENT TOKEN = TOKEN_KEYWORD_DEF

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_LEFTPAR);

    if (parse_param_set() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURR. TOKEN = TOKEN_RIGHTPAR

    if (parse_return_type() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURR. TOKEN = TOKEN_ASSIGN

    return ERR_OK;
}

ErrorCode parse_main_anot() {
    GET_TOKEN();
    if (token.type == TOKEN_KEYWORD_DEF) {
        return ERR_OK;
    }
    /*CHECK_TOKEN(TOKEN_KEYWORD_MAIN); -- TODO*/

    if (parse_main_anot_eol() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // Current token = TOKEN_KEYWORD_DEF
}

ErrorCode parse_main_anot_eol() {
    GET_TOKEN();
    if (token.type == TOKEN_EOL) {
        GET_TOKEN();
        CHECK_TOKEN(TOKEN_KEYWORD_DEF);
        return ERR_OK;
    }

    if (token.type == TOKEN_KEYWORD_DEF) {
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

ErrorCode parse_param_set() {
    GET_TOKEN();
    if (token.type == TOKEN_RIGHTPAR) {
        return ERR_OK;
    }

    if (parse_param() != ERR_OK) {
        return ERR_SYNTAX;
    }

    if (parse_param_next() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // CURRENT TOKEN = TOKEN_RIGHTPAR
}

ErrorCode parse_param() {
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    if (parse_type() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // CURR. TOKEN = TOKEN_~TYPE
}

ErrorCode parse_param_next() {
    GET_TOKEN();
    if (token.type == TOKEN_RIGHTPAR) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COMMA);

    GET_TOKEN();
    if (parse_param() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    if (parse_param_next() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK;
}

ErrorCode parse_type() {
    switch (token.type) {
    case TOKEN_KEYWORD_INT:
        return ERR_OK;
        break;
    case TOKEN_KEYWORD_STRING:
        return ERR_OK;
        break;
    case TOKEN_KEYWORD_DOUBLE:
        return ERR_OK;
        break;
    case TOKEN_KEYWORD_UNIT:
        return ERR_OK;
        break;
    case TOKEN_KEYWORD_BOOLEAN:
        return ERR_OK;
        break;
    default:
        return ERR_SYNTAX;
        break;
    }
}

ErrorCode parse_return_type() {
    GET_TOKEN();
    if (token.type == TOKEN_ASSIGN) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    if (parse_type() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_ASSIGN);
    return ERR_OK; // CURR. TOKEN = TOKEN_ASSIGN
}

ErrorCode parse_body() {
    GET_TOKEN();
    if (token.type == TOKEN_EOL) {
        if (parse_block() != ERR_OK) {
            return ERR_SYNTAX;
        }
    } else {
        /*&token - ..., &symtable - ..., &arg_type - type of function where it's called */
        if (/* parse_expression(&token, &symtable, &arg_type) != ERR_OK */ false /* TODO */) {
            return ERR_SYNTAX;
        }
    }

    return ERR_OK;
}

ErrorCode parse_block() {
    GET_TOKEN();
    CHECK_TOKEN(TOKEN_INDENT);

    /* TODO */

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_DEDENT);
    return ERR_OK;
}

ErrorCode parse_command_set() {
    if (token.type == TOKEN_DEDENT) {
        return ERR_OK;
    }

    /* TODO */

    return ERR_OK;
}
