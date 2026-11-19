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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARAM_ARRAY_SIZE 20

Token token;
Token *peek_token;
SymTable *symtable;

VarInfo var_info;
char *var_name;
FuncInfo func_info;

static bool unparsed_token = false;
static bool is_main_func = false;
static size_t main_count = 0;

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

#define CHECK_SYM(something)                                                                       \
    do {                                                                                           \
        if (something != ERR_OK) {                                                                 \
            return something;                                                                      \
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

void init_parser() {
    init_arrays();
    func_info_reset();
    unparsed_token = false;
    is_main_func = false;
    main_count = 0;
}

void clean_symtable() {
    func_info_reset();
    symtable_scope_pop(symtable);
}

static bool same_types(Type type_expect, Type type_current) {
    if (type_expect == type_current) {
        return true;
    }
    if (type_expect == TYPE_DOUBLE && type_current == TYPE_INT) {
        return true;
    }

    return false;
}

static block_result init_block_result() {
    block_result block;

    block.result_type = TYPE_UNIT;
    block.unit_branch = true;
    block.is_statement = false;
    block.returns_always = false;
    block.last_statement = STAT_NONE;

    return block;
}

ErrorCode fill_symtable(SymTable *symtable) {
    ErrorCode sym_err;
    sym_err = symtable_add_func(symtable, "print", 1, (Type[]){TYPE_ANY}, (const char *[]){"value"},
                                TYPE_UNIT, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "readLine", 0, NULL, NULL, TYPE_STRING, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "readInt", 0, NULL, NULL, TYPE_INT, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "readDouble", 0, NULL, NULL, TYPE_DOUBLE, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "readBoolean", 0, NULL, NULL, TYPE_BOOL, true);
    CHECK_SYM(sym_err);

    sym_err = symtable_add_func(symtable, "toStr", 1, (Type[]){TYPE_ANY}, (const char *[]){"value"},
                                TYPE_STRING, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "toInt", 1, (Type[]){TYPE_DOUBLE},
                                (const char *[]){"value"}, TYPE_INT, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "strLength", 1, (Type[]){TYPE_STRING},
                                (const char *[]){"string"}, TYPE_INT, true);
    CHECK_SYM(sym_err);

    sym_err = symtable_add_func(symtable, "subStr", 3, (Type[]){TYPE_STRING, TYPE_INT, TYPE_INT},
                                (const char *[]){"s", "start", "length"}, TYPE_STRING, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "ord", 2, (Type[]){TYPE_STRING, TYPE_INT},
                                (const char *[]){"s", "index"}, TYPE_INT, true);
    CHECK_SYM(sym_err);
    sym_err = symtable_add_func(symtable, "chr", 1, (Type[]){TYPE_INT}, (const char *[]){"i"},
                                TYPE_STRING, true);
    CHECK_SYM(sym_err);

    return ERR_OK;
}

// 1st run
ErrorCode parse_func_heads() {
    ErrorCode err;
    err = parse_main_anot();
    if (err != ERR_OK) {
        return err;
    }

    if (main_count > 1) {
        return ERR_SEM_REDEF;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    func_info.name = strdup(token.value.str_value.data);
    if (func_info.name == NULL) {
        return ERR_INTERNAL;
    }

    if (!symtable_scope_push(symtable)) {
        func_info_reset();
        return ERR_INTERNAL;
    }

    // GET_TOKEN with symtable memeory clean
    if (unparsed_token) {
        unparsed_token = false;
    } else {
        token_free(&token);
        err = get_next_token(&token);
        if (err != ERR_OK) {
            clean_symtable();
            return err;
        }
    }

    // CHECK_TOKEN with clean memory
    if (token.type == TOKEN_LEFTPAR) {
        err = parse_param_set();
        if (err != ERR_OK) {
            clean_symtable();
            return err;
        } // CURR. TOKEN = TOKEN_RIGHTPAR
        // Check for LIT_UNIT TOKEN
    } else if (token.type == TOKEN_LIT_UNIT) {
        func_info.param_count = 0;
    } else {
        clean_symtable();
        return ERR_SYNTAX;
    }

    // Main function (no parameter)
    if (is_main_func && func_info.param_count != 0) {
        clean_symtable();
        return ERR_SEM_ARGS;
    }

    err = parse_return_type();
    if (err != ERR_OK) {
        clean_symtable();
        return err;
    } // CURR. TOKEN = TOKEN_ASSIGN

    err = symtable_add_func(symtable, func_info.name, func_info.param_count, func_info.param_types,
                            func_info.param_names, func_info.return_type, false);

    return err;
}

ErrorCode skip_func_body() {
    GET_TOKEN();

    if (token.type == TOKEN_EOL) {
        GET_TOKEN();
        CHECK_TOKEN(TOKEN_INDENT);

        int depth = 1;

        while (depth > 0) {
            GET_TOKEN();

            if (token.type == TOKEN_EOF) {
                return ERR_SYNTAX;
            }

            if (token.type == TOKEN_INDENT) {
                depth++;
            } else if (token.type == TOKEN_DEDENT) {
                depth--;
            }
        }

        return ERR_OK;
    } else {
        if (token.type == TOKEN_EOF) {
            return ERR_SYNTAX;
        }

        while (token.type != TOKEN_EOL && token.type != EOF) {
            GET_TOKEN();
        }

        return ERR_OK;
    }
}

ErrorCode parse_func_set_heads() {
    if (token.type == TOKEN_EOF) {
        return ERR_OK;
    }

    ErrorCode err = parse_func_heads();
    if (err != ERR_OK) {
        return err;
    }

    err = skip_func_body();
    if (err != ERR_OK) {
        return err;
    }

    GET_TOKEN();

    err = parse_func_set_heads();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

ErrorCode parse_program() {

    // Initialization
    symtable = symtable_init();
    if (symtable == NULL) {
        return ERR_INTERNAL;
    }
    ErrorCode err = fill_symtable(symtable);
    if (err != ERR_OK) {
        return ERR_INTERNAL;
    }
    init_parser();

    // Core
    GET_TOKEN();
    err = parse_prolog();
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

    // No Main?
    if (main_count == 0) {
        symtable_free(symtable);
        free_arrays();
        // Test-2012
        printf("No @main\n");
        return ERR_SEM_UNDEF;
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

    if (strcmp(token.value.str_value.data, "ifj26") != 0) {
        return ERR_SYNTAX;
    }

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

ErrorCode parse_func() {

    ErrorCode err = parse_main_anot();
    if (err != ERR_OK) {
        return err;
    } // CURRENT TOKEN = TOKEN_KEYWORD_DEF

    if (main_count > 1) {
        return ERR_SEM_REDEF;
    }

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_IDENTIFIER);

    func_info.name = strdup(token.value.str_value.data);
    if (func_info.name == NULL) {
        return ERR_INTERNAL;
    }

    if (!symtable_scope_push(symtable)) {
        func_info_reset();
        return ERR_INTERNAL;
    }

    // GET_TOKEN with symtable memeory clean
    if (unparsed_token) {
        unparsed_token = false;
    } else {
        token_free(&token);
        err = get_next_token(&token);
        if (err != ERR_OK) {
            clean_symtable();
            return err;
        }
    }

    // CHECK_TOKEN with clean memory
    if (token.type == TOKEN_LEFTPAR) {
        err = parse_param_set();
        if (err != ERR_OK) {
            clean_symtable();
            return err;
        } // CURR. TOKEN = TOKEN_RIGHTPAR
        // Check for LIT_UNIT TOKEN
    } else if (token.type == TOKEN_LIT_UNIT) {
        func_info.param_count = 0;
    } else {
        clean_symtable();
        return ERR_SYNTAX;
    }

    // Main function (no parameter)
    if (is_main_func && func_info.param_count != 0) {
        clean_symtable();
        return ERR_SEM_ARGS;
    }

    err = parse_return_type();
    if (err != ERR_OK) {
        clean_symtable();
        return err;
    } // CURR. TOKEN = TOKEN_ASSIGN

    err = parse_body();
    if (err != ERR_OK) {
        clean_symtable();
        return err;
    } // CURR. TOKEN = TOKEN_DEDENT

    // Check final funtion type
    if (is_main_func && func_info.return_type != TYPE_UNIT) {
        clean_symtable();
        return ERR_SEM_TYPES;
    }

    FuncInfo *found_func = symtable_find_func(symtable, func_info.name);
    if (found_func == NULL) {
        clean_symtable();
        // Test - 2012
        printf("No func found\n");
        return ERR_SEM_UNDEF;
    }

    // return_type form function body
    found_func->return_type = func_info.return_type;
    found_func->is_defined = true;

    clean_symtable();
    return ERR_OK;
}

ErrorCode parse_main_anot() {
    if (token.type == TOKEN_KEYWORD_DEF) {
        is_main_func = false;
        return ERR_OK;
    }
    CHECK_TOKEN(TOKEN_MAIN);
    is_main_func = true;
    main_count = main_count + 1;

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
    ErrorCode err;
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

    // Freeing ID when error
    if (unparsed_token) {
        unparsed_token = false;
    } else {
        token_free(&token);
        err = get_next_token(&token);
        if (err != ERR_OK) {
            free(id);
            return err;
        }
    }
    if (token.type != TOKEN_COLON) {
        free(id);
        return ERR_SYNTAX;
    }

    // GET_TOKEN with symtable memeory clean
    if (unparsed_token) {
        unparsed_token = false;
    } else {
        token_free(&token);
        err = get_next_token(&token);
        if (err != ERR_OK) {
            free(id);
            return err;
        }
    }

    err = parse_type(&param_type);
    if (err != ERR_OK) {
        free(id);
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

    int comma_line = token.line;

    GET_TOKEN();

    if (token.type == TOKEN_RIGHTPAR) {
        if (token.line > comma_line) {
            return ERR_OK;
        }
        return ERR_SYNTAX;
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
    block_result result = init_block_result();

    GET_TOKEN();
    if (token.type == TOKEN_EOL) {
        err = parse_block(&result);
        if (err != ERR_OK) {
            return err;
        }

        // RETURN
        if (result.returns_always) {
            if (func_info.return_type == TYPE_UNKNOWN) {
                return ERR_SEM_TYPES;
            }

            return ERR_OK;
        }

        // IF stat
        if (result.last_statement == STAT_IF) {
            if (func_info.return_type == TYPE_UNIT) {
                return ERR_OK;
            }

            if (func_info.return_type == TYPE_UNKNOWN && result.unit_branch) {
                func_info.return_type = TYPE_UNIT;
                return ERR_OK;
            }

            return ERR_SEM_TYPES;
        }

        if (func_info.return_type == TYPE_UNKNOWN) {
            func_info.return_type = result.result_type;
        } else if (func_info.return_type != TYPE_UNIT &&
                   !same_types(func_info.return_type, result.result_type)) {
            return ERR_SEM_TYPES;
        }
    } else {
        Type decared_type = func_info.return_type;
        Type expression_type = TYPE_UNKNOWN;

        err = parse_expression(&token, symtable, &expression_type);
        if (err != ERR_OK) {
            return err;
        } // Curr. Token = TOKEN_EOL

        if (decared_type != TYPE_UNKNOWN && decared_type != TYPE_UNIT &&
            !same_types(decared_type, expression_type)) {
            return ERR_SEM_TYPES;
        }

        if (decared_type == TYPE_UNKNOWN) {
            func_info.return_type = expression_type;
        } else {
            func_info.return_type = decared_type;
        }

        CHECK_TOKEN(TOKEN_EOL);
    }

    return ERR_OK; // Curr. Token = TOKEN_EOL
}

ErrorCode parse_block(block_result *result) {
    *result = init_block_result();
    block_result command_result = init_block_result();

    GET_TOKEN();
    CHECK_TOKEN(TOKEN_INDENT);

    GET_TOKEN();
    ErrorCode err = parse_command(&command_result);
    if (err != ERR_OK) {
        return err;
    }

    if (command_result.is_statement) {
        *result = command_result;
    }

    err = parse_command_set(result);
    if (err != ERR_OK) {
        return err;
    }

    if (!result->is_statement) {
        return ERR_SYNTAX;
    }

    return ERR_OK; // CURR. TOKEN = TOKEN_DEDENT
}

ErrorCode parse_command_set(block_result *result) {
    block_result command_result = init_block_result();

    GET_TOKEN();
    if (token.type == TOKEN_DEDENT) {
        return ERR_OK;
    }

    ErrorCode err = parse_command(&command_result);
    if (err != ERR_OK) {
        return err;
    }

    if (command_result.is_statement) {
        bool prev_return = result->returns_always;
        *result = command_result;

        if (prev_return || result->returns_always) {
            result->returns_always = true;
        } else {
            result->returns_always = false;
        }
    }

    err = parse_command_set(result);
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

ErrorCode parse_command(block_result *result) {
    *result = init_block_result();

    if (token.type == TOKEN_EOL) {
        return ERR_OK;
    }

    ErrorCode err;

    if (token.type == TOKEN_KEYWORD_IF || token.type == TOKEN_KEYWORD_WHILE) {
        err = parse_command_complex(result);
        return err;
    }

    err = parse_command_simple(result);
    if (err != ERR_OK) {
        return err;
    }
    err = parse_separator();
    if (err != ERR_OK) {
        return err;
    }

    return ERR_OK;
}

ErrorCode parse_command_simple(block_result *result) {
    ErrorCode err;
    Type expression_type = TYPE_UNKNOWN;

    *result = init_block_result();
    result->is_statement = true;
    result->last_statement = STAT_OTHER;

    switch (token.type) {
    case TOKEN_KEYWORD_VAR:
        err = parse_variable();
        if (err != ERR_OK) {
            return err;
        }

        result->result_type = TYPE_UNIT;
        result->last_statement = STAT_OTHER;

        break;

    case TOKEN_KEYWORD_VAL:
        err = parse_value();
        if (err != ERR_OK) {
            return err;
        }

        result->result_type = TYPE_UNIT;
        result->last_statement = STAT_OTHER;

        break;

    case TOKEN_KEYWORD_RETURN:
        GET_TOKEN();

        if (func_info.return_type == TYPE_UNKNOWN) {
            return ERR_SEM_TYPES;
        }

        // Is it Unit function is the question
        if (token.type == TOKEN_EOL || token.type == TOKEN_DEDENT ||
            token.type == TOKEN_SEMICOLON) {
            if (func_info.return_type != TYPE_UNIT) {
                return ERR_SEM_TYPES;
            }

            result->last_statement = STAT_RETURN;
            result->returns_always = true;
            unparsed_token = true;
            break;
        }

        Type declaration_type = func_info.return_type;
        expression_type = TYPE_UNKNOWN;

        err = parse_expression(&token, symtable, &expression_type);
        if (err != ERR_OK) {
            return err;
        }

        // Hanging on eol
        unparsed_token = true;

        if (declaration_type != TYPE_UNIT && !same_types(declaration_type, expression_type)) {
            return ERR_SEM_TYPES;
        }

        result->last_statement = STAT_RETURN;
        result->returns_always = true;
        break;

    case TOKEN_IDENTIFIER:
        var_name = strdup(token.value.str_value.data);
        if (var_name == NULL) {
            return ERR_INTERNAL;
        }

        err = peek_next_token(&peek_token);
        if (err != ERR_OK) {
            free(var_name);
            var_name = NULL;
            return err;
        }

        err = parse_command_simple_id(result);
        if (err != ERR_OK) {
            free(var_name);
            var_name = NULL;
            return err;
        }
        break;
    case TOKEN_KEYWORD_TRUE:
    case TOKEN_KEYWORD_FALSE:
    case TOKEN_LIT_INT:
    case TOKEN_LIT_DOUBLE:
    case TOKEN_LIT_STRING:
    case TOKEN_LIT_UNIT:
    case TOKEN_LEFTPAR:
    case TOKEN_MINUS:
    case TOKEN_NOT:
        expression_type = TYPE_UNKNOWN;
        err = parse_expression(&token, symtable, &expression_type);
        if (err != ERR_OK) {
            return err;
        }

        result->result_type = expression_type;
        result->last_statement = STAT_EXPRESSION;
        unparsed_token = true;

        break;
    default:
        return ERR_SYNTAX;
    }

    // Cleaning
    free(var_name);
    var_name = NULL;

    return ERR_OK;
}

ErrorCode parse_command_simple_id(block_result *result) {
    ErrorCode err;
    Type expression_type = TYPE_UNKNOWN;

    // ID = Expression
    if (peek_token->type == TOKEN_ASSIGN) {

        VarInfo *found_var = symtable_find_var(symtable, var_name);
        if (found_var == NULL) {
            // Test - 2012
            printf("No variable or val found\n");
            return ERR_SEM_UNDEF;
        }

        if (found_var->is_mutable) {
            // 1.gt ASSSIGN -> 2.gt litTOKEN
            GET_TOKEN();
            GET_TOKEN();

            err = parse_expression(&token, symtable, &expression_type);
            if (err != ERR_OK) {
                return err;
            }

            // Hanging on eol
            unparsed_token = true;

            if (!same_types(found_var->type, expression_type)) {
                return ERR_SEM_TYPES;
            }

            result->result_type = TYPE_UNIT;
            result->last_statement = STAT_OTHER;
            return ERR_OK;
        }

        return ERR_SEM_ASSIGN;
    }

    // FuncCall or expression (alone)
    err = parse_expression(&token, symtable, &expression_type);
    if (err != ERR_OK) {
        return err;
    }

    unparsed_token = true;

    result->result_type = expression_type;
    result->last_statement = STAT_EXPRESSION;
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
        var_name = NULL;
        return err;
    }

    Type decalared_type = var_info.type;
    Type expression_type = TYPE_UNKNOWN;

    GET_TOKEN();
    err = parse_expression(&token, symtable, &expression_type);
    if (err != ERR_OK) {
        free(var_name);
        var_name = NULL;
        return err;
    }

    // Hanging on eol or separator
    unparsed_token = true;

    if (decalared_type != TYPE_UNKNOWN && !same_types(decalared_type, expression_type)) {
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
        var_name = NULL;
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
        var_name = NULL;
        return err;
    }

    Type decalared_type = var_info.type;
    Type expression_type = TYPE_UNKNOWN;

    GET_TOKEN();
    err = parse_expression(&token, symtable, &expression_type);
    if (err != ERR_OK) {
        free(var_name);
        var_name = NULL;
        return err;
    }

    // Hanging on eol or semicolon
    unparsed_token = true;

    if (decalared_type != TYPE_UNKNOWN && !same_types(decalared_type, expression_type)) {
        free(var_name);
        var_name = NULL;
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
        var_name = NULL;
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

ErrorCode parse_nested_blocks(block_result *result) {
    *result = init_block_result();

    if (!symtable_scope_push(symtable)) {
        return ERR_INTERNAL;
    }

    ErrorCode err = parse_block(result);

    symtable_scope_pop(symtable);

    return err;
}

ErrorCode parse_command_complex(block_result *result) {
    ErrorCode err;
    *result = init_block_result();

    if (token.type == TOKEN_KEYWORD_IF) {
        err = parse_if_statement(result);
        if (err != ERR_OK) {
            return err;
        }

        result->last_statement = STAT_IF;
        result->is_statement = true;

        return ERR_OK;
    }

    if (token.type == TOKEN_KEYWORD_WHILE) {
        err = parse_while_statement(result);
        if (err != ERR_OK) {
            return err;
        }

        result->last_statement = STAT_WHILE;
        result->result_type = TYPE_UNIT;
        result->is_statement = true;

        return ERR_OK;
    }

    return ERR_SYNTAX;
}

ErrorCode parse_if_statement(block_result *result) {
    ErrorCode err;
    Type condition_type = TYPE_UNKNOWN;

    block_result then_result = init_block_result();
    block_result else_result = init_block_result();

    *result = init_block_result();

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

    err = parse_nested_blocks(&then_result);
    if (err != ERR_OK) {
        return err;
    } // Curr. TOKEN == TOKEN_DEDENT

    GET_TOKEN();
    err = parse_vol_else_block(&else_result);
    if (err != ERR_OK) {
        return err;
    } // Curr. TOKEN == TOKEN_DEDENT

    if (then_result.returns_always && else_result.returns_always) {
        result->returns_always = true;
    } else {
        result->returns_always = false;
    }

    bool unit_then;
    if (then_result.last_statement == STAT_IF) {
        unit_then = then_result.unit_branch;
    } else {
        if (then_result.result_type == TYPE_UNIT) {
            unit_then = true;
        } else {
            unit_then = false;
        }
    }

    bool unit_else;
    if (else_result.last_statement == STAT_IF) {
        unit_else = else_result.unit_branch;
    } else {
        if (else_result.result_type == TYPE_UNIT) {
            unit_else = true;
        } else {
            unit_else = false;
        }
    }

    if (unit_else && unit_then) {
        result->unit_branch = true;
    } else {
        result->unit_branch = false;
    }

    result->result_type = TYPE_UNIT;
    result->last_statement = STAT_IF;
    result->is_statement = true;

    return ERR_OK; // Curr. TOKEN == TOKEN_DEDENT
}

ErrorCode parse_vol_else_block(block_result *result) {
    ErrorCode err;
    *result = init_block_result();

    if (token.type != TOKEN_KEYWORD_ELSE) {
        unparsed_token = true;
        return ERR_OK;
    }

    GET_TOKEN();

    // Just ELSE / EOL - branch
    if (token.type == TOKEN_EOL) {
        err = parse_nested_blocks(result);
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }
    // ELSE IF / IF - branch
    if (token.type == TOKEN_KEYWORD_IF) {
        err = parse_if_statement(result);
        if (err != ERR_OK) {
            return err;
        }
        return ERR_OK;
    }

    return ERR_SYNTAX; // Curr. TOKEN == TOKEN_DEDENT
}

ErrorCode parse_while_statement(block_result *result) {
    ErrorCode err;
    Type condition_type = TYPE_UNKNOWN;
    block_result while_result = init_block_result();

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

    err = parse_nested_blocks(&while_result);
    if (err != ERR_OK) {
        return err;
    }

    result->returns_always = false;
    result->result_type = TYPE_UNIT;
    result->unit_branch = true;
    result->last_statement = STAT_WHILE;
    result->is_statement = true;

    return ERR_OK;
}
