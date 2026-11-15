/**
 * File: parser.c
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#include "parser.h"
#include "error.h"
#include "precedence.h"
#include "scanner.h"
#include "symtable.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PARAM_ARRAY_SIZE 20

Token token;
SymTable *symtable;

static bool unparsed_token = false;

VarInfo var_info;
char *var_name;
FuncInfo func_info;

#define GET_TOKEN()                                                                                \
    do {                                                                                           \
        if (unparsed_token) {                                                                      \
            unparsed_token = false;                                                                \
        } else {                                                                                   \
            token_free(&token);                                                                    \
            ErrorCode get_token_err = get_next_token(&token);                                      \
            if (get_token_err != ERR_OK) {                                                         \
                return get_token_err;                                                              \
            }                                                                                      \
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

void func_info_reset() {
    free((void *)func_info.name);
    func_info.name = NULL;

    for (size_t i = 0; i < func_info.param_count; i++) {
        free(param_names_buffer[i]);
        param_names_buffer[i] = NULL;
    }
    func_info.param_count = 0;
    func_info.return_type = TYPE_UNKNOWN;
}

void free_arrays() {
    func_info_reset();

    free(param_names_buffer);
    free(func_info.param_types);

    param_names_buffer = NULL;
    func_info.param_names = NULL;
    func_info.param_types = NULL;
    param_capacity = PARAM_ARRAY_SIZE;
}

ErrorCode parse_program() {

    // Initialization
    symtable = symtable_init();
    if (symtable == NULL) {
        return ERR_INTERNAL;
    }
    init_arrays();
    func_info_reset();

    // Core
    GET_TOKEN();
    ErrorCode err = parse_prolog();
    if (err != ERR_OK) {
        symtable_free(symtable);
        free_arrays();
        return err;
    }

    GET_TOKEN();
    err = parse_func_set();
    if (err != ERR_OK) {
        symtable_free(symtable);
        free_arrays();
        return err;
    }

    if (token.type != TOKEN_EOF) {
        symtable_free(symtable);
        free_arrays();
        return ERR_SYNTAX;
    }

    // Cleaning
    symtable_free(symtable);
    free_arrays();

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

    ErrorCode err = parse_func();
    if (err != ERR_OK) {
        return err;
    }

    GET_TOKEN();

    err = parse_func_set();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

void clean_symtable() {
    func_info_reset();
    symtable_scope_pop(symtable);
}

ErrorCode parse_func() {

    ErrorCode err = parse_main_anot();
    if (err != ERR_OK) {
        return err;
    } // CURRENT TOKEN = TOKEN_KEYWORD_DEF

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    // Maybe I will have to use my own function
    func_info.name = strdup(token.value.str_value.data);
    if (func_info.name == NULL) {
        return ERR_INTERNAL;
    }

    if (!symtable_scope_push(symtable)) {
        return ERR_INTERNAL;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_LEFTPAR);

    err = parse_param_set();
    if (err != ERR_OK) {
        clean_symtable();
        return err;
    } // CURR. TOKEN = TOKEN_RIGHTPAR

    err = parse_return_type();
    if (err != ERR_OK) {
        clean_symtable();
        return err;
    } // CURR. TOKEN = TOKEN_ASSIGN

    ErrorCode sym_err =
        symtable_add_func(symtable, func_info.name, func_info.param_count, func_info.param_types,
                          func_info.param_names, func_info.return_type, true);

    if (sym_err != ERR_OK) {
        clean_symtable();
        return sym_err;
    }

    err = parse_body();
    if (err != ERR_OK) {
        clean_symtable();
        return err;
    } // CURR. TOKEN = TOKEN_DEDENT

    clean_symtable();
    return ERR_OK;
}

ErrorCode parse_main_anot() {
    if (token.type == TOKEN_KEYWORD_DEF) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_MAIN);

    ErrorCode err = parse_main_anot_eol();
    if (err != ERR_OK) {
        return err;
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

    ErrorCode err = parse_param();
    if (err != ERR_OK) {
        return err;
    } // Curr. TOKEN == TOKEN~_TYPE

    err = parse_param_next();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK; // CURRENT TOKEN = TOKEN_RIGHTPAR
}

ErrorCode parse_param() {
    Type param_type = TYPE_UNKNOWN;
    // Check if array is full
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
            error_exit(ERR_INTERNAL, "%d: Failed to realloc param_names (Parser)\n", ERR_INTERNAL);
        }

        param_names_buffer = names_arr;
        func_info.param_names = (const char *const *)param_names_buffer;
        param_capacity = new_capacity;
    }

    CHECK_TOKEN(TOKEN_IDENTIFIER);
    // Get param name
    char *id = strdup(token.value.str_value.data);
    if (id == NULL) {
        return ERR_INTERNAL;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_COLON);

    GET_TOKEN();
    ErrorCode err = parse_type(&param_type);
    if (err != ERR_OK) {
        return err;
    }

    ErrorCode sym_err = symtable_add_var(symtable, id, param_type, false);
    if (sym_err != ERR_OK) {
        free(id);
        return sym_err;
    }

    // Fill param array with attributes
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

    ErrorCode err = parse_param();
    if (err != ERR_OK) {
        return err;
    } // Curr. TOKEN == TOKEN_~TYPE

    err = parse_param_next();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

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
    ErrorCode err = parse_type(&func_info.return_type);
    if (err != ERR_OK) {
        return err;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_ASSIGN);
    return ERR_OK; // CURR. TOKEN = TOKEN_ASSIGN
}

ErrorCode parse_body() {
    ErrorCode err;
    GET_TOKEN();
    if (token.type == TOKEN_EOL) {
        err = parse_block();
        if (err != ERR_OK) {
            return err;
        }
    } else {
        err = parse_expression(&token, symtable, &func_info.return_type);
        /*&token - ..., &symtable - ..., &arg_type - type of function where it's called */
        if (err != ERR_OK) {
            return err;
        } // Curr. Token = TOKEN_EOL

        CHECK_TOKEN(TOKEN_EOL);
    }

    return ERR_OK; // Curr. Token = TOKEN_EOL
}

ErrorCode parse_block() {
    GET_TOKEN();
    CHECK_TOKEN(TOKEN_INDENT);

    GET_TOKEN();
    ErrorCode err = parse_command();
    if (err != ERR_OK) {
        return err;
    }

    err = parse_command_set();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK; // CURR. TOKEN = TOKEN_DEDENT
}

ErrorCode parse_command_set() {
    GET_TOKEN();
    if (token.type == TOKEN_DEDENT) {
        return ERR_OK;
    }

    ErrorCode err = parse_command();
    if (err != ERR_OK) {
        return err;
    }

    err = parse_command_set();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

ErrorCode parse_command() {
    if (token.type == TOKEN_EOL) {
        return ERR_OK;
    }

    ErrorCode err;

    if (token.type == TOKEN_KEYWORD_IF || token.type == TOKEN_KEYWORD_WHILE) {
        err = parse_command_complex();
        return err;
    }

    err = parse_command_simple();
    if (err != ERR_OK) {
        return err;
    }
    err = parse_separator();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

// Parse Expresion TODO
ErrorCode parse_command_simple() {
    ErrorCode err;
    switch (token.type) {
    case TOKEN_KEYWORD_VAR:
        err = parse_variable();
        if (err != ERR_OK) {
            return err;
        }
        break;

    case TOKEN_KEYWORD_VAL:
        err = parse_value();
        if (err != ERR_OK) {
            return err;
        }
        break;

    // TODO HERE ->
    case TOKEN_KEYWORD_RETURN:
        GET_TOKEN();

        Type expression_type = TYPE_UNKNOWN;

        err = parse_expression(&token, symtable, &expression_type);
        if (err != ERR_OK) {
            return err;
        }

        // Hanging on eol
        unparsed_token = true;

        if (func_info.return_type != TYPE_UNKNOWN && expression_type != func_info.return_type) {
            return ERR_SEM_TYPES;
        }

        break;

    case TOKEN_IDENTIFIER:
        var_name = strdup(token.value.str_value.data);
        if (var_name == NULL) {
            return ERR_INTERNAL;
        }

        GET_TOKEN();
        err = parse_command_simple_id();
        if (err != ERR_OK) {
            free(var_name);
            var_name = NULL;
            return err;
        }
        break;

    default:
        return ERR_SYNTAX;
    }

    // Cleaning
    free(var_name);
    var_name = NULL;

    return ERR_OK;
}

// TODO - ID () - funcCall (parse_expression can't do this)
ErrorCode parse_command_simple_id() {
    ErrorCode err;

    // ID = Expression
    if (token.type == TOKEN_ASSIGN) {
        GET_TOKEN();

        VarInfo *found_var = symtable_find_var(symtable, var_name);
        if (found_var == NULL) {
            return ERR_SEM_UNDEF;
        }

        Type expression_type = TYPE_UNKNOWN;

        if (found_var->is_mutable) {
            err = parse_expression(&token, symtable, &expression_type);
            if (err != ERR_OK) {
                return err;
            }

            // Hanging on eol
            unparsed_token = true;

            if (expression_type != found_var->type) {
                return ERR_SEM_TYPES;
            }

            return ERR_OK;
        }

        return ERR_SEM_ASSIGN;
    }

    // ID ( ARG_SET ) - TODO
    if (token.type == TOKEN_LEFTPAR) {
        err = parse_arg_set();
        if (err != ERR_OK) {
            return err;
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

    ErrorCode err = parse_term();
    if (err != ERR_OK) {
        return err;
    }

    GET_TOKEN();
    err = parse_arg_next();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

ErrorCode parse_arg_next() {
    if (token.type == TOKEN_RIGHTPAR) {
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_COMMA);

    GET_TOKEN();
    ErrorCode err = parse_term();
    if (err != ERR_OK) {
        return err;
    } // Curren. TOKEN == TOKEN_~TERM

    GET_TOKEN();
    err = parse_arg_next();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

ErrorCode parse_variable() {
    var_info.is_mutable = true;

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    var_name = strdup(token.value.str_value.data);
    if (var_name == NULL) {
        return ERR_INTERNAL;
    }

    ErrorCode err = parse_variable_type();
    if (err != ERR_OK) {
        free(var_name);
        return err;
    }

    Type decalared_type = var_info.type;
    Type expression_type = TYPE_UNKNOWN;

    GET_TOKEN();
    err = parse_expression(&token, symtable, &expression_type);
    if (err != ERR_OK) {
        free(var_name);
        return err;
    }

    // Hanging on eol or separator
    unparsed_token = true;

    if (decalared_type != TYPE_UNKNOWN && decalared_type != expression_type) {
        free(var_name);
        var_name = NULL;
        return ERR_SEM_TYPES;
    }

    if (decalared_type == TYPE_UNKNOWN) {
        var_info.type = expression_type;
    } else {
        var_info.type = decalared_type;
    }

    ErrorCode sym_err = symtable_add_var(symtable, var_name, var_info.type, var_info.is_mutable);
    if (sym_err != ERR_OK) {
        free(var_name);
        return sym_err;
    }

    // Cleaning
    free(var_name);
    var_name = NULL;

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
    ErrorCode err = parse_type(&var_info.type);
    if (err != ERR_OK) {
        return err;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_ASSIGN);
    return ERR_OK; // CURR. TOKEN = TOKEN_ASSIGN
}

ErrorCode parse_value() {
    var_info.is_mutable = false;

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    var_name = strdup(token.value.str_value.data);
    if (var_name == NULL) {
        return ERR_INTERNAL;
    }

    ErrorCode err = parse_variable_type();
    if (err != ERR_OK) {
        free(var_name);
        return err;
    }

    Type decalared_type = var_info.type;
    Type expression_type = TYPE_UNKNOWN;

    GET_TOKEN();
    err = parse_expression(&token, symtable, &expression_type);
    if (err != ERR_OK) {
        free(var_name);
        return err;
    }

    // Hanging on eol or semicolon
    unparsed_token = true;

    if (decalared_type != TYPE_UNKNOWN && decalared_type != expression_type) {
        free(var_name);
        return ERR_SEM_TYPES;
    }

    // What type it should be
    if (decalared_type == TYPE_UNKNOWN) {
        var_info.type = expression_type;
    } else {
        var_info.type = decalared_type;
    }

    ErrorCode sym_err = symtable_add_var(symtable, var_name, var_info.type, var_info.is_mutable);
    if (sym_err != ERR_OK) {
        free(var_name);
        return sym_err;
    }

    // Cleaning
    free(var_name);
    var_name = NULL;

    return ERR_OK;
}

ErrorCode parse_separator() {
    GET_TOKEN();
    if (token.type == TOKEN_SEMICOLON) {
        return ERR_OK;
    } else if (token.type == TOKEN_EOL) {
        return ERR_OK;
    }

    if (token.type == TOKEN_DEDENT) {
        unparsed_token = true;
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

ErrorCode parse_nested_blocks() {
    if (!symtable_scope_push(symtable)) {
        return ERR_INTERNAL;
    }

    ErrorCode err = parse_block();

    symtable_scope_pop(symtable);

    return err;
}

ErrorCode parse_command_complex() {
    ErrorCode err;
    if (token.type == TOKEN_KEYWORD_IF) {
        err = parse_if_statement();
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }

    if (token.type == TOKEN_KEYWORD_WHILE) {
        err = parse_while_statement();
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }

    return ERR_SYNTAX;
}

/* TODO - parse_expression() and else block maybe done*/
ErrorCode parse_if_statement() {
    ErrorCode err;
    Type condition_type = TYPE_UNKNOWN;

    GET_TOKEN();
    err = parse_expression(&token, symtable, &condition_type);
    if (err != ERR_OK) {
        return err;
    }

    if (condition_type != TYPE_BOOL) {
        return ERR_SEM_TYPES;
    }

    // Already has token
    CHECK_TOKEN(TOKEN_KEYWORD_THEN);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOL);

    err = parse_nested_blocks();
    if (err != ERR_OK) {
        return err;
    } // Curr. TOKEN == TOKEN_DEDENT

    GET_TOKEN();
    err = parse_vol_else_block();
    if (err != ERR_OK) {
        return err;
    } // Curr. TOKEN == TOKEN_DEDENT

    return ERR_OK; // Curr. TOKEN == TOKEN_DEDENT
}

ErrorCode parse_vol_else_block() {
    ErrorCode err;
    if (token.type != TOKEN_KEYWORD_ELSE) {
        unparsed_token = true;
        return ERR_OK;
    }

    GET_TOKEN();

    // Just ELSE / EOL - branch
    if (token.type == TOKEN_EOL) {
        err = parse_nested_blocks();
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }
    // ELSE IF / IF - branch
    if (token.type == TOKEN_KEYWORD_IF) {
        err = parse_if_statement();
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }

    return ERR_SYNTAX; // Curr. TOKEN == TOKEN_DEDENT
}

ErrorCode parse_while_statement() {
    ErrorCode err;
    Type condition_type = TYPE_UNKNOWN;

    GET_TOKEN();
    err = parse_expression(&token, symtable, &condition_type);
    if (err != ERR_OK) {
        return err;
    }

    if (condition_type != TYPE_BOOL) {
        return ERR_SEM_TYPES;
    }

    // Token si read by expression
    CHECK_TOKEN(TOKEN_KEYWORD_DO);

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_EOL);

    err = parse_nested_blocks();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}
