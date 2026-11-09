/**
 * File: parser.c
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#include "parser.h"
#include "error.h"
#include "scanner.h"
#include "symtable.h"
#include <stdbool.h>

Token token;
SymTable *symtable;

#define GET_TOKEN()                                                                                \
    do {                                                                                           \
        token_free(&token);                                                                        \
        if (get_next_token(&token) != ERR_OK) {                                                    \
            return ERR_LEXICAL;                                                                    \
        }                                                                                          \
        if (&token == NULL) {                                                                      \
            return ERR_INTERNAL;                                                                   \
        }                                                                                          \
    } while (0)

#define CHECK_TOKEN(something)                                                                     \
    do {                                                                                           \
        if (&token == NULL) {                                                                      \
            return ERR_INTERNAL;                                                                   \
        }                                                                                          \
        if (token.type != something) {                                                             \
            return ERR_SYNTAX;                                                                     \
        }                                                                                          \
    } while (0)

ErrorCode parse_program() {

    symtable = symtable_init();

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

    if (parse_body() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURR. TOKEN = TOKEN_DEDENT

    return ERR_OK;
}

/* TODO - Implement Token keyword MAIN */
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
    } // Curr. TOKEN == TOKEN_RIGHTPAR

    if (parse_param() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curr. TOKEN == TOKEN~_TYPE

    if (parse_param_next() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // CURRENT TOKEN = TOKEN_RIGHTPAR
}

ErrorCode parse_param() {
    if (token.type == TOKEN_EOL) {
        GET_TOKEN();
    }
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    if (parse_type() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // CURR. TOKEN = TOKEN_~TYPE
}

ErrorCode parse_vol_eol() {
    if (token.type == TOKEN_IDENTIFIER || token.type == TOKEN_EOL) {
        return ERR_OK;
    }
    return ERR_SYNTAX;
}

ErrorCode parse_param_next() {
    GET_TOKEN();
    if (token.type == TOKEN_RIGHTPAR) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COMMA);

    GET_TOKEN();
    if (parse_vol_eol() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curr. TOKEN == EOL || ID

    GET_TOKEN();
    if (parse_param() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curr. TOKEN == TOKEN_~TYPE

    GET_TOKEN();
    if (parse_param_next() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curr. TOKEN == TOKEN_~TYPE

    return ERR_OK;
}

/* TODO - get return type for parse_expression() */
ErrorCode parse_type() {
    switch (token.type) {
    case TOKEN_KEYWORD_INT:
    case TOKEN_KEYWORD_STRING:
    case TOKEN_KEYWORD_DOUBLE:
    case TOKEN_KEYWORD_UNIT:
    case TOKEN_KEYWORD_BOOLEAN:
        break;
    default:
        return ERR_SYNTAX;
    }

    return ERR_OK;
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

/* TODO - finish parse_expression() */
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

    if (parse_command() != ERR_OK) {
        return ERR_SYNTAX;
    }

    if (parse_command_set() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // CURR. TOKEN = TOKEN_DEDENT
}

ErrorCode parse_command_set() {
    GET_TOKEN();
    if (token.type == TOKEN_DEDENT) {
        return ERR_OK;
    }

    if (parse_command() != ERR_OK) {
        return ERR_SYNTAX;
    }

    if (parse_command_set() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK;
}

ErrorCode parse_command() {
    if (parse_command_complex() == ERR_OK) {
        return ERR_OK;
    }

    if (parse_command_simple() == ERR_OK && parse_separator() == ERR_OK) {
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

/* TODO - ... */
ErrorCode parse_command_simple() {
    switch (token.type) {
    case TOKEN_EOL:
        break;
    case TOKEN_KEYWORD_VAR:
        if (parse_variable() == ERR_OK) {
            break;
        }
        return ERR_SYNTAX;

    case TOKEN_KEYWORD_VAL:
        if (parse_value() == ERR_OK) {
            break;
        }
        return ERR_SYNTAX;

    case TOKEN_KEYWORD_RETURN:
        GET_TOKEN();
        if (/* parse_expression(&token, &symtable, &arg_type) == ERR_OK */ false /* TODO */) {
            break;
        }
        return ERR_SYNTAX;

    case TOKEN_IDENTIFIER:
        GET_TOKEN();
        CHECK_TOKEN(TOKEN_ASSIGN);

        GET_TOKEN();
        if (/* parse_expression(&token, &symtable, &arg_type) == ERR_OK */ false /* TODO */) {
            break;
        }
        return ERR_SYNTAX;

    default:
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

ErrorCode parse_variable() {
    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    if (parse_variable_type() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    if (/* parse_expression(&token, &symtable, &arg_type) != ERR_OK */ false /* TODO */) {
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

/* Do I really need this OR should I just use: parse_ret_type() */
ErrorCode parse_variable_type() {
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

ErrorCode parse_value() {
    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    if (parse_variable_type() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    if (/* parse_expression(&token, &symtable, &arg_type) != ERR_OK */ false /* TODO */) {
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

ErrorCode parse_separator() {
    GET_TOKEN();
    if (token.type == TOKEN_SEMICOLON) {
        return ERR_OK;
    } else if (token.type == TOKEN_EOL) {
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

/* TODO - add IF and WHILE statements */
ErrorCode parse_command_complex() {
    if (token.type == TOKEN_KEYWORD_IF) {
        if (parse_if_statement() == ERR_OK) {
            return ERR_OK;
        }
    }

    if (token.type == TOKEN_KEYWORD_WHILE) {
    }

    return ERR_SYNTAX;
}

/* TODO - parse_expression() and else block maybe done*/
ErrorCode parse_if_statement() {
    GET_TOKEN();

    if (/* parse_expression(&token, &symtable, &arg_type) != ERR_OK */ false /* TODO */) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_KEYWORD_THEN);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOL);

    if (parse_block() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curr. TOKEN == TOKEN_DEDENT

    GET_TOKEN();
    if (parse_vol_else_block() != ERR_OK) {
        return ERR_SYNTAX; // Curr. TOKEN == TOKEN_DEDENT
    }

    return ERR_OK; // Curr. TOKEN == TOKEN_DEDENT
}

/* TODO - Maybe not idk */
ErrorCode parse_vol_else_block() {
    if (token.type == TOKEN_KEYWORD_ELSE) {
        GET_TOKEN(); // IF || EOL
        // Just ELSE / EOL - branch
        if (token.type == TOKEN_EOL) {
            if (parse_block() != ERR_OK) {
                return ERR_SYNTAX;
            }
        }
        // ELSE IF / IF - branch
        if (token.type == TOKEN_KEYWORD_IF) {
            if (parse_if_statement() != ERR_OK) {
                return ERR_SYNTAX;
            }
        }
    }

    return ERR_OK; // Curr. TOKEN == TOKEN_DEDENT
}

ErrorCode parse_whlile_statement() {
    GET_TOKEN();
    if (/* parse_expression(&token, &symtable, &arg_type) != ERR_OK */ false /* TODO */) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_KEYWORD_DO);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOL);

    if (parse_block() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK;
}
