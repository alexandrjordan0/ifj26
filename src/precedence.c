/**
 * File: precedence.c
 * Project: IFJ26
 * Authors: Alexandr Jordán (xjordaa00)
 * Translator: gcc 16.2.1
 */

#include "precedence.h"
#include <stdlib.h>

typedef enum {
    CONV_NONE,     // no conversion
    CONV_LEFT_INT, // store top and convert to float
    CONV_RIGHT_INT // convert top to float
} ConversionType;

typedef enum {
    ACT_SHIFT, // <
    ACT_EQUAL, // =
    ACT_REDUC, // >
    ACT_ERROR
} PrecAction;

#define PREC_TABLE_SIZE (PREC_SYM_DOLLAR + 1)

// TODO: NAVRHNOUT TABULKU A PREPSAT
static const PrecAction prec_table[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
    // TODO
    [0 ... PREC_TABLE_SIZE - 1][0 ... PREC_TABLE_SIZE - 1] = ACT_ERROR};

static void stack_init(PrecStack *stack) {
    if (stack == NULL) {
        return;
    }

    stack->top = NULL;
}

static void stack_free(PrecStack *stack) {
    if (stack == NULL) {
        return;
    }

    PrecStackItem *current = stack->top;
    while (current != NULL) {
        PrecStackItem *tmp = current;
        current = current->next;
        free(tmp);
    }

    stack->top = NULL;
}

static bool stack_push(PrecStack *stack, PrecSymbol symbol, Type data_type) {
    if (stack == NULL) {
        return false;
    }

    PrecStackItem *new_top = malloc(sizeof(PrecStackItem));
    if (new_top == NULL) {
        return false;
    }

    new_top->symbol = symbol;
    new_top->data_type = data_type;
    new_top->next = stack->top;
    stack->top = new_top;

    return true;
}

static void stack_pop(PrecStack *stack) {
    if (stack == NULL || stack->top == NULL) {
        return;
    }

    PrecStackItem *tmp = stack->top;
    stack->top = stack->top->next;
    free(tmp);
}

static PrecStackItem *stack_top_terminal(PrecStack *stack) {
    if (stack == NULL) {
        return NULL;
    }

    PrecStackItem *current = stack->top;
    while (current != NULL) {
        if (current->symbol != PREC_SYM_NONTERM && current->symbol != PREC_SYM_STOP) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

static bool stack_insert_stop_after_top_terminal(PrecStack *stack) {
    if (stack == NULL) {
        return false;
    }

    PrecStackItem *prev = NULL;
    PrecStackItem *curr = stack->top;

    while (curr != NULL && curr->symbol == PREC_SYM_NONTERM) {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL) {
        return false;
    }

    PrecStackItem *stop_item = malloc(sizeof(PrecStackItem));
    if (stop_item == NULL) {
        return false;
    }

    stop_item->symbol = PREC_SYM_STOP;
    stop_item->data_type = TYPE_UNKNOWN;

    if (prev == NULL) {
        stop_item->next = stack->top;
        stack->top = stop_item;

        return true;
    }

    stop_item->next = curr;
    prev->next = stop_item;

    return true;
}

static ErrorCode token_to_prec_sym(Token *token, SymTable *sym_table, PrecSymbol *out_sym,
                                   Type *out_type, int *out_depth) {
    if (token == NULL || sym_table == NULL || out_sym == NULL || out_type == NULL ||
        out_depth == NULL) {
        return ERR_INTERNAL;
    }

    *out_type = TYPE_UNKNOWN;
    *out_depth = 0;

    switch (token->type) {

    // +,-,*,/
    case TOKEN_PLUS:
        *out_sym = PREC_SYM_PLUS;
        break;
    case TOKEN_MINUS:
        *out_sym = PREC_SYM_MINUS;
        break;
    case TOKEN_MUL:
        *out_sym = PREC_SYM_MUL;
        break;
    case TOKEN_DIV:
        *out_sym = PREC_SYM_DIV;
        break;

    // logic
    case TOKEN_EQ:
        *out_sym = PREC_SYM_EQ;
        break;
    case TOKEN_NEQ:
        *out_sym = PREC_SYM_NEQ;
        break;
    case TOKEN_LESSTHAN:
        *out_sym = PREC_SYM_LT;
        break;
    case TOKEN_GREATERTHAN:
        *out_sym = PREC_SYM_GT;
        break;
    case TOKEN_LESSOREQ:
        *out_sym = PREC_SYM_LTE;
        break;
    case TOKEN_GREATEROREQ:
        *out_sym = PREC_SYM_GTE;
        break;
    case TOKEN_NOT:
        *out_sym = PREC_SYM_NOT;
        break;
    case TOKEN_AND:
        *out_sym = PREC_SYM_AND;
        break;
    case TOKEN_OR:
        *out_sym = PREC_SYM_OR;
        break;

    // brackets
    case TOKEN_LEFTPAR:
        *out_sym = PREC_SYM_LPAR;
        break;
    case TOKEN_RIGHTPAR:
        *out_sym = PREC_SYM_RPAR;
        break;

    // literals
    case TOKEN_LIT_INT:
        *out_sym = PREC_SYM_ID;
        *out_type = TYPE_INT;
        break;
    case TOKEN_LIT_DOUBLE:
        *out_sym = PREC_SYM_ID;
        *out_type = TYPE_DOUBLE;
        break;
    case TOKEN_LIT_STRING:
        *out_sym = PREC_SYM_ID;
        *out_type = TYPE_STRING;
        break;
    case TOKEN_LIT_UNIT:
        *out_sym = PREC_SYM_ID;
        *out_type = TYPE_UNIT;
        break;
    case TOKEN_KEYWORD_TRUE:
    case TOKEN_KEYWORD_FALSE:
        *out_sym = PREC_SYM_ID;
        *out_type = TYPE_BOOL;
        break;

    // identifier
    case TOKEN_IDENTIFIER: {
        VarInfo *var = symtable_find_var(sym_table, token->value.str_value.data);

        if (var == NULL) {
            return ERR_SEM_UNDEF;
        }

        if (var->type == TYPE_UNKNOWN) {
            return ERR_SEM_TYPES;
        }

        *out_sym = PREC_SYM_ID;
        *out_type = var->type;
        *out_depth = var->scope_depth;
        break;
    }

    // any other token terminates expression
    default:
        *out_sym = PREC_SYM_DOLLAR;
        *out_type = TYPE_UNKNOWN;
        break;
    }

    return ERR_OK;
}

static ErrorCode check_binary_types(PrecStackItem *expr1, PrecSymbol op, PrecStackItem *expr2,
                                    Type *res_type, ConversionType *out_conv) {
    if (expr1 == NULL || expr2 == NULL || res_type == NULL || out_conv == NULL) {
        return ERR_INTERNAL;
    }

    *res_type = TYPE_UNKNOWN;
    *out_conv = CONV_NONE;

    Type type1 = expr1->data_type;
    Type type2 = expr2->data_type;

    switch (op) {
    case PREC_SYM_PLUS:
        // string concate
        if (type1 == TYPE_STRING && type2 == TYPE_STRING) {
            *res_type = TYPE_STRING;
            return ERR_OK;
        }
        // for number continue to shared logic with - and *
        [[fallthrough]];
    case PREC_SYM_MINUS:
    case PREC_SYM_MUL:
        if (type1 == TYPE_INT && type2 == TYPE_INT) {
            *res_type = TYPE_INT;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_DOUBLE) {
            *res_type = TYPE_DOUBLE;
            return ERR_OK;
        }
        if (type1 == TYPE_INT && type2 == TYPE_DOUBLE) {
            *out_conv = CONV_LEFT_INT;
            *res_type = TYPE_DOUBLE;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_INT) {
            *out_conv = CONV_RIGHT_INT;
            *res_type = TYPE_DOUBLE;
            return ERR_OK;
        }
        return ERR_SEM_TYPES;

    case PREC_SYM_DIV:
        if (type1 == TYPE_INT && type2 == TYPE_INT) {
            // IDIVS
            *res_type = TYPE_INT;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_DOUBLE) {
            *res_type = TYPE_DOUBLE;
            return ERR_OK;
        }
        if (type1 == TYPE_INT && type2 == TYPE_DOUBLE) {
            *out_conv = CONV_LEFT_INT;
            *res_type = TYPE_DOUBLE;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_INT) {
            *out_conv = CONV_RIGHT_INT;
            *res_type = TYPE_DOUBLE;
            return ERR_OK;
        }
        return ERR_SEM_TYPES;

    case PREC_SYM_LT:
    case PREC_SYM_LTE:
    case PREC_SYM_GT:
    case PREC_SYM_GTE:
        if (type1 == TYPE_INT && type2 == TYPE_INT) {
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_DOUBLE) {
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        if (type1 == TYPE_INT && type2 == TYPE_DOUBLE) {
            *out_conv = CONV_LEFT_INT;
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_INT) {
            *out_conv = CONV_RIGHT_INT;
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        if (type1 == TYPE_STRING && type2 == TYPE_STRING) {
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        return ERR_SEM_TYPES;

    case PREC_SYM_EQ:
    case PREC_SYM_NEQ:
        // same types
        if (type1 == type2 && type1 != TYPE_UNKNOWN) {
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        if (type1 == TYPE_INT && type2 == TYPE_DOUBLE) {
            *out_conv = CONV_LEFT_INT;
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        if (type1 == TYPE_DOUBLE && type2 == TYPE_INT) {
            *out_conv = CONV_RIGHT_INT;
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        return ERR_SEM_TYPES;

    // BOOLTHEN
    case PREC_SYM_AND:
    case PREC_SYM_OR:
        if (type1 == TYPE_BOOL && type2 == TYPE_BOOL) {
            *res_type = TYPE_BOOL;
            return ERR_OK;
        }
        return ERR_SEM_TYPES;

    default:
        return ERR_SEM_TYPES;
    }

    return ERR_OK;
}
