/**
 * File: precedence.c
 * Project: IFJ26
 * Authors: Alexandr Jordán (xjordaa00)
 * Translator: gcc 16.2.1
 */

#include "precedence.h"
#include "codegenerator.h"
#include <stdlib.h>

typedef enum {
    CONV_NONE,     // no conversion
    CONV_LEFT_INT, // left operand is second from top, convert it to float
    CONV_RIGHT_INT // right operand is on top, convert it to float
} ConversionType;

typedef enum {
    ACT_ERROR = 0,
    ACT_SHIFT, // <
    ACT_EQUAL, // =
    ACT_REDUCE // >
} PrecAction;

#define PREC_TABLE_SIZE (PREC_SYM_DOLLAR + 1)

// TODO: NAVRHNOUT TABULKU A PREPSAT
static const PrecAction prec_table[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {0};

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

static bool is_binary_op(PrecSymbol symbol) {
    switch (symbol) {
    case PREC_SYM_PLUS:
    case PREC_SYM_MINUS:
    case PREC_SYM_MUL:
    case PREC_SYM_DIV:
    case PREC_SYM_EQ:
    case PREC_SYM_NEQ:
    case PREC_SYM_LT:
    case PREC_SYM_GT:
    case PREC_SYM_LTE:
    case PREC_SYM_GTE:
    case PREC_SYM_AND:
    case PREC_SYM_OR:
        return true;
    default:
        return false;
    }
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
        // for number continue to shared logic with - , * and /
        [[fallthrough]];
    case PREC_SYM_MINUS:
    case PREC_SYM_MUL:
    case PREC_SYM_DIV:
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
}

static ErrorCode reduce(PrecStack *stack) {
    if (stack == NULL || stack->top == NULL) {
        return ERR_INTERNAL;
    }

    PrecStackItem *items[3] = {NULL, NULL, NULL};
    int count = 0;

    PrecStackItem *current = stack->top;
    while (current != NULL && current->symbol != PREC_SYM_STOP) {
        if (count < 3) {
            items[count] = current;
        }
        count++;
        current = current->next;
    }

    if (current == NULL || count < 1 || count > 3) {
        return ERR_SYNTAX;
    }

    Type result_type = TYPE_UNKNOWN;

    switch (count) {
    case 1:
        // E -> i
        if (items[0]->symbol != PREC_SYM_ID) {
            return ERR_SYNTAX;
        }
        result_type = items[0]->data_type;
        break;
    case 2:
        // E -> ! E
        if (items[1]->symbol == PREC_SYM_NOT && items[0]->symbol == PREC_SYM_NONTERM) {
            if (items[0]->data_type != TYPE_BOOL) {
                return ERR_SEM_TYPES;
            }
            result_type = TYPE_BOOL;
            // TODO: codegen_not();
            break;
        }
        return ERR_SYNTAX;
    case 3:
        if (items[2]->symbol == PREC_SYM_LPAR && items[1]->symbol == PREC_SYM_NONTERM &&
            items[0]->symbol == PREC_SYM_RPAR) {
            // E -> ( E )
            result_type = items[1]->data_type;
            break;
        } else if (items[2]->symbol == PREC_SYM_NONTERM && is_binary_op(items[1]->symbol) &&
                   items[0]->symbol == PREC_SYM_NONTERM) {
            // E-> E op E
            PrecSymbol op = items[1]->symbol;
            ConversionType conv = CONV_NONE;

            ErrorCode err = check_binary_types(items[2], op, items[0], &result_type, &conv);
            if (err != ERR_OK) {
                return err;
            }

            // TODO codegen conv a op
            break;
        }
        return ERR_SYNTAX;
    default:
        return ERR_SYNTAX;
    }

    // cleanup
    for (int i = 0; i < count; i++) {
        stack_pop(stack);
    }
    stack_pop(stack);

    if (!stack_push(stack, PREC_SYM_NONTERM, result_type)) {
        return ERR_INTERNAL;
    }

    return ERR_OK;
}

ErrorCode parse_expression(Token *current_token, SymTable *symtable, Type *result_type) {
    if (current_token == NULL || symtable == NULL || result_type == NULL) {
        return ERR_INTERNAL;
    }

    PrecStack prec_stack;
    PrecStack *stack = &prec_stack;
    stack_init(stack);

    if (!stack_push(stack, PREC_SYM_DOLLAR, TYPE_UNKNOWN)) {
        return ERR_INTERNAL;
    }

    ErrorCode status = ERR_OK;

    while (true) {
        PrecStackItem *top_term = stack_top_terminal(stack);
        if (top_term == NULL) {
            stack_free(stack);
            return ERR_INTERNAL;
        }

        PrecSymbol input_sym = PREC_SYM_DOLLAR;
        Type input_type = TYPE_UNKNOWN;
        int input_depth = 0;
        status = token_to_prec_sym(current_token, symtable, &input_sym, &input_type, &input_depth);
        if (status != ERR_OK) {
            stack_free(stack);
            return status;
        }

        if (top_term->symbol == PREC_SYM_DOLLAR && input_sym == PREC_SYM_DOLLAR) {
            break;
        }

        PrecAction act = prec_table[top_term->symbol][input_sym];

        switch (act) {
        // <
        case ACT_SHIFT:
            if (!stack_insert_stop_after_top_terminal(stack)) {
                stack_free(stack);
                return ERR_INTERNAL;
            }

            // if literal or identificator, generate PUSH
            if (input_sym == PREC_SYM_ID) {
                if (current_token->type == TOKEN_IDENTIFIER) {
                    gen_push_var(current_token->value.str_value.data, FRAME_LF, input_depth);
                } else {
                    gen_push_literal(current_token);
                }
            }

            // add input symbol to top
            if (!stack_push(stack, input_sym, input_type)) {
                stack_free(stack);
                return ERR_INTERNAL;
            }

            // free current token and read new from scanner
            token_free(current_token);
            status = get_next_token(current_token);
            if (status != ERR_OK) {
                stack_free(stack);
                return status;
            }

            break;
        // =
        case ACT_EQUAL:
            // add input symbol to top
            if (!stack_push(stack, input_sym, input_type)) {
                stack_free(stack);
                return ERR_INTERNAL;
            }

            // free current token and read new from scanner
            token_free(current_token);
            status = get_next_token(current_token);
            if (status != ERR_OK) {
                stack_free(stack);
                return status;
            }

            break;
        // >
        case ACT_REDUCE:
            // call reduce and check status
            status = reduce(stack);
            if (status != ERR_OK) {
                stack_free(stack);
                return status;
            }

            break;

        // covers ACT_ERROR
        default:
            stack_free(stack);
            return ERR_SYNTAX;
        }
    }

    // check results
    if (stack->top == NULL || stack->top->next == NULL || stack->top->next->next != NULL) {
        stack_free(stack);
        return ERR_SYNTAX;
    }

    if (stack->top->symbol != PREC_SYM_NONTERM || stack->top->next->symbol != PREC_SYM_DOLLAR) {
        status = ERR_SYNTAX;
    } else {
        *result_type = stack->top->data_type;
    }

    // free stack
    stack_free(stack);
    return status;
}
