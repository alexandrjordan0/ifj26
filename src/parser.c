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
#include <stdlib.h>
#include <string.h>

#define PARAM_ARRAY_SIZE 20

Token token;
SymTable *symtable;

VarInfo var_info;
FuncInfo func_info;

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

static char **param_names_buffer = NULL;
static size_t param_capacity = PARAM_ARRAY_SIZE;
void init_arrays() {
    Type *type_arr = malloc(sizeof(*type_arr) * param_capacity);
    if (type_arr == NULL) {
        error_exit(ERR_INTERNAL, "%d: Failed to allocate param_types (Parser)\n", ERR_INTERNAL);
    }

    char **name_arr = calloc(param_capacity, sizeof(*name_arr));
    if (name_arr == NULL) {
        free(type_arr);
        error_exit(ERR_INTERNAL, "%d: Failed to allocate param_names (Parser)\n", ERR_INTERNAL);
    }

    param_names_buffer = name_arr;

    func_info.param_types = type_arr;
    func_info.param_names = (const char *const *)name_arr;
    func_info.param_count = 0;
}

ErrorCode parse_program() {

    // Initialization
    symtable = symtable_init();
    init_arrays();

    // Core
    GET_TOKEN();
    if (parse_prolog() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    if (parse_func_set() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOF);

    // Cleaning
    symtable_free(symtable);

    return ERR_OK;
}

ErrorCode parse_prolog() {
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
    if (token.type == TOKEN_EOF) {
        return ERR_OK;
    }

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

    // Maybe I will have to use my own function
    func_info.name = strdup(token.value.str_value.data);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_LEFTPAR);

    if (parse_param_set() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURR. TOKEN = TOKEN_RIGHTPAR

    if (parse_return_type() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURR. TOKEN = TOKEN_ASSIGN

    symtable_add_func(symtable, func_info.name, func_info.param_count,
                        func_info.param_types, func_info.param_names, func_info.return_type, false);

    if (parse_body() != ERR_OK) {
        return ERR_SYNTAX;
    } // CURR. TOKEN = TOKEN_DEDENT

    return ERR_OK;
}

/* TODO - Implement Token keyword MAIN */
ErrorCode parse_main_anot() {
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
        func_info.param_count = 0;
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
    Type param_type = TYPE_UNKNOWN;
    if (func_info.param_count == param_capacity) {
        size_t new_capacity = 2 * param_capacity;

        Type *type_arr =
            realloc(func_info.param_types, sizeof(*func_info.param_types) * new_capacity);
        if (type_arr == NULL) {
            error_exit(ERR_INTERNAL, "%d: Failed to realloc param_types (Parser)\n", ERR_INTERNAL);
        }

        func_info.param_types = type_arr;

        char **names_arr = realloc(param_names_buffer, sizeof(*param_names_buffer) * new_capacity);
        if (names_arr == NULL) {
            free(type_arr);
            error_exit(ERR_INTERNAL, "%d: Failed to realloc param_names (Parser)\n", ERR_INTERNAL);
        }

        param_names_buffer = names_arr;
        func_info.param_names = (const char *const *)param_names_buffer;
        param_capacity = new_capacity;
    }

    CHECK_TOKEN(TOKEN_IDENTIFIER);
    char *id = strdup(token.value.str_value.data);
    if (id == NULL) {
        return ERR_INTERNAL;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    if (parse_type(&param_type) != ERR_OK) {
        return ERR_SYNTAX;
    }

    func_info.param_types[func_info.param_count] = param_type;
    param_names_buffer[func_info.param_count] = id;
    func_info.param_count++;
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
    if (token.type == TOKEN_EOL) {
        GET_TOKEN();
    }

    if (parse_param() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curr. TOKEN == TOKEN_~TYPE

    if (parse_param_next() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK;
}

/* TODO - get return type for parse_expression() */
ErrorCode parse_type(Type *type) {
    switch (token.type) {
    case TOKEN_KEYWORD_INT:
        *type = TYPE_INT;
        break;
    case TOKEN_KEYWORD_STRING:
        *type = TYPE_STRING;
        break;
    case TOKEN_KEYWORD_DOUBLE:
        *type = TYPE_DOUBLE;
        break;
    case TOKEN_KEYWORD_UNIT:
        *type = TYPE_UNIT;
        break;
    case TOKEN_KEYWORD_BOOLEAN:
        *type = TYPE_BOOL;
        break;
    default:
        return ERR_SYNTAX;
    }

    return ERR_OK;
}

ErrorCode parse_return_type() {
    func_info.return_type = TYPE_UNKNOWN;

    GET_TOKEN();
    if (token.type == TOKEN_ASSIGN) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    if (parse_type(&func_info.return_type) != ERR_OK) {
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

    GET_TOKEN();
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
    if (token.type == TOKEN_EOL) {
        return ERR_OK;
    }

    if (token.type == TOKEN_KEYWORD_IF || token.type == TOKEN_KEYWORD_WHILE) {
        return parse_command_complex();
    }

    if (parse_command_simple() == ERR_OK && parse_separator() == ERR_OK) {
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

/* TODO - ... */
ErrorCode parse_command_simple() {
    switch (token.type) {
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
        if (parse_command_simple_id() == ERR_OK) {
            break;
        }
        return ERR_SYNTAX;

    default:
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

ErrorCode parse_command_simple_id() {
    if (token.type == TOKEN_ASSIGN) {
        GET_TOKEN();
        if (/* parse_expression(...) != ERR_OK */ false) {
            return ERR_SYNTAX;
        }
        return ERR_OK;
    }

    if (token.type == TOKEN_LEFTPAR) {
        if (parse_arg_set() != ERR_OK) {
            return ERR_SYNTAX;
        }
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

ErrorCode parse_term() {
    switch (token.type) {
    case TOKEN_IDENTIFIER:
        break;
    case TOKEN_LIT_INT:
        break;
    case TOKEN_LIT_DOUBLE:
        break;
    case TOKEN_LIT_STRING:
        break;
    case TOKEN_LIT_UNIT:
        break;
    case TOKEN_KEYWORD_TRUE:
        break;
    case TOKEN_KEYWORD_FALSE:
        break;
    default:
        return ERR_SYNTAX;
    }
    return ERR_OK;
}

ErrorCode parse_arg_set() {
    GET_TOKEN();
    if (token.type == TOKEN_RIGHTPAR) {
        return ERR_OK;
    }

    if (parse_term() != ERR_OK) {
        return ERR_SYNTAX;
    }

    GET_TOKEN();
    if (parse_arg_next() != ERR_OK) {
        return ERR_SYNTAX;
    }

    return ERR_OK;
}

ErrorCode parse_arg_next() {
    if (token.type == TOKEN_RIGHTPAR) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COMMA);

    GET_TOKEN();
    if (parse_term() != ERR_OK) {
        return ERR_SYNTAX;
    } // Curren. TOKEN == TOKEN_~TERM

    GET_TOKEN();
    if (parse_arg_next() != ERR_OK) {
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

ErrorCode parse_variable_type() {
    var_info.type = TYPE_UNKNOWN;

    GET_TOKEN();
    if (token.type == TOKEN_ASSIGN) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    if (parse_type(&var_info.type) != ERR_OK) {
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

ErrorCode parse_command_complex() {
    if (token.type == TOKEN_KEYWORD_IF) {
        if (parse_if_statement() != ERR_OK) {
            return ERR_SYNTAX;
        }
        return ERR_OK;
    }

    if (token.type == TOKEN_KEYWORD_WHILE) {
        if (parse_while_statement() != ERR_OK) {
            return ERR_SYNTAX;
        }
        return ERR_OK;
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
    if (token.type != TOKEN_KEYWORD_ELSE) {
        return ERR_OK;
    }

    GET_TOKEN();

    // Just ELSE / EOL - branch
    if (token.type == TOKEN_EOL) {
        if (parse_block() == ERR_OK) {
            return ERR_OK;
        }
    }
    // ELSE IF / IF - branch
    if (token.type == TOKEN_KEYWORD_IF) {
        if (parse_if_statement() == ERR_OK) {
            return ERR_OK;
            ;
        }
    }

    return ERR_SYNTAX; // Curr. TOKEN == TOKEN_DEDENT
}

ErrorCode parse_while_statement() {
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
