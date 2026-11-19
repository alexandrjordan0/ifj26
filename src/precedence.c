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

// label id of shifted && or || waiting for reduction
typedef struct LogicLabel {
    int label_id;
    struct LogicLabel *next;
} LogicLabel;

// && and || are reduced in reverse order of shifting, last shifted is on top
static LogicLabel *logic_labels = NULL;

#define PREC_TABLE_SIZE (PREC_SYM_DOLLAR + 1)

#define S ACT_SHIFT
#define E ACT_EQUAL
#define R ACT_REDUCE
#define X ACT_ERROR

static const PrecAction prec_table[PREC_TABLE_SIZE][PREC_TABLE_SIZE] = {
    /*       +  -  *  / == !=  <  > <= >=  ! -u && ||  (  )  i  $ */
    /* +  */ {R, R, S, S, R, R, R, R, R, R, S, S, R, R, S, R, S, R},
    /* -  */ {R, R, S, S, R, R, R, R, R, R, S, S, R, R, S, R, S, R},
    /* *  */ {R, R, R, R, R, R, R, R, R, R, S, S, R, R, S, R, S, R},
    /* /  */ {R, R, R, R, R, R, R, R, R, R, S, S, R, R, S, R, S, R},
    /* == */ {S, S, S, S, X, X, S, S, S, S, S, S, R, R, S, R, S, R},
    /* != */ {S, S, S, S, X, X, S, S, S, S, S, S, R, R, S, R, S, R},
    /* <  */ {S, S, S, S, R, R, X, X, X, X, S, S, R, R, S, R, S, R},
    /* >  */ {S, S, S, S, R, R, X, X, X, X, S, S, R, R, S, R, S, R},
    /* <= */ {S, S, S, S, R, R, X, X, X, X, S, S, R, R, S, R, S, R},
    /* >= */ {S, S, S, S, R, R, X, X, X, X, S, S, R, R, S, R, S, R},
    /* !  */ {R, R, R, R, R, R, R, R, R, R, S, S, R, R, S, R, S, R},
    /* -u */ {R, R, R, R, R, R, R, R, R, R, S, S, R, R, S, R, S, R},
    /* && */ {S, S, S, S, S, S, S, S, S, S, S, S, R, R, S, R, S, R},
    /* || */ {S, S, S, S, S, S, S, S, S, S, S, S, S, R, S, R, S, R},
    /* (  */ {S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, E, S, X},
    /* )  */ {R, R, R, R, R, R, R, R, R, R, X, X, R, R, X, R, X, R},
    /* i  */ {R, R, R, R, R, R, R, R, R, R, X, X, R, R, X, R, X, R},
    /* $  */ {S, S, S, S, S, S, S, S, S, S, S, S, S, S, S, X, S, X},
};

#undef S
#undef E
#undef R
#undef X

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

static bool logic_label_push(int label_id) {
    LogicLabel *item = malloc(sizeof(LogicLabel));
    if (item == NULL) {
        return false;
    }

    item->label_id = label_id;
    item->next = logic_labels;
    logic_labels = item;

    return true;
}

static bool logic_label_pop(int *label_id) {
    if (logic_labels == NULL) {
        return false;
    }

    LogicLabel *item = logic_labels;
    if (label_id != NULL) {
        *label_id = item->label_id;
    }
    logic_labels = item->next;
    free(item);

    return true;
}

static ErrorCode token_to_prec_sym(Token *token, PrecSymbol *out_sym, Type *out_type) {
    if (token == NULL || out_sym == NULL || out_type == NULL) {
        return ERR_INTERNAL;
    }

    *out_type = TYPE_UNKNOWN;

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

    // identifier, resolved in shift
    case TOKEN_IDENTIFIER:
        *out_sym = PREC_SYM_ID;
        break;

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
            gen_expr_op(PREC_SYM_NOT, TYPE_BOOL);
            break;
        }
        // E -> - E
        if (items[1]->symbol == PREC_SYM_UMINUS && items[0]->symbol == PREC_SYM_NONTERM) {
            if (items[0]->data_type != TYPE_INT && items[0]->data_type != TYPE_DOUBLE) {
                return ERR_SEM_TYPES;
            }
            result_type = items[0]->data_type;
            gen_expr_op(PREC_SYM_UMINUS, result_type);
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

            if (op == PREC_SYM_AND || op == PREC_SYM_OR) {
                // left operand was tested when operator was shifted
                int label_id = 0;
                if (!logic_label_pop(&label_id)) {
                    return ERR_INTERNAL;
                }
                gen_logic_end(op, label_id);
                break;
            }

            if (conv == CONV_LEFT_INT) {
                gen_conv_second_int2float();
            } else if (conv == CONV_RIGHT_INT) {
                gen_conv_top_int2float();
            }

            Type operand_type = conv != CONV_NONE ? TYPE_DOUBLE : items[2]->data_type;
            gen_expr_op(op, operand_type);
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

// FUNEXP
static ErrorCode parse_call(const char *name, Token *current_token, SymTable *symtable,
                            Type *ret_type) {
    if (name == NULL || current_token == NULL || symtable == NULL || ret_type == NULL) {
        return ERR_INTERNAL;
    }

    *ret_type = TYPE_UNKNOWN;

    FuncInfo *func = symtable_find_func(symtable, name);
    if (func == NULL) {
        return ERR_SEM_UNDEF;
    }

    ErrorCode status = ERR_OK;
    size_t arg_count = 0;

    // call without arguments
    if (current_token->type == TOKEN_LIT_UNIT) {
        token_free(current_token);
        status = get_next_token(current_token);
        if (status != ERR_OK) {
            return status;
        }
    } else {
        if (current_token->type != TOKEN_LEFTPAR) {
            return ERR_SYNTAX;
        }

        token_free(current_token);
        status = get_next_token(current_token);
        if (status != ERR_OK) {
            return status;
        }

        // parse arguments
        while (true) {
            Type arg_type = TYPE_UNKNOWN;
            status = parse_expression(current_token, symtable, &arg_type);
            if (status != ERR_OK) {
                return status;
            }

            if (func != NULL) {
                if (arg_count >= func->param_count) {
                    return ERR_SEM_ARGS;
                }

                Type param_type = func->param_types[arg_count];
                if (param_type == TYPE_DOUBLE && arg_type == TYPE_INT) {
                    // argument is on top of stack
                    gen_conv_top_int2float();
                } else if (param_type != TYPE_ANY && param_type != arg_type) {
                    return ERR_SEM_ARGS;
                }
            }
            arg_count++;

            if (current_token->type == TOKEN_COMMA) {
                int comma_line = current_token->line;

                token_free(current_token);
                status = get_next_token(current_token);
                if (status != ERR_OK) {
                    return status;
                }

                // trailing comma is allowed only if ) is on a later line
                if (current_token->type == TOKEN_RIGHTPAR && current_token->line > comma_line) {
                    token_free(current_token);
                    status = get_next_token(current_token);
                    if (status != ERR_OK) {
                        return status;
                    }
                    break;
                }
                continue;
            }

            if (current_token->type == TOKEN_RIGHTPAR) {
                token_free(current_token);
                status = get_next_token(current_token);
                if (status != ERR_OK) {
                    return status;
                }
                break;
            }

            return ERR_SYNTAX;
        }
    }

    if (func != NULL) {
        if (arg_count != func->param_count) {
            return ERR_SEM_ARGS;
        }

        // TODO function without return type
        if (func->return_type == TYPE_UNKNOWN) {
            return ERR_SEM_TYPES;
        }

        *ret_type = func->return_type;
    }

    gen_call(name);

    return ERR_OK;
}

static ErrorCode parse_variable(const char *name, SymTable *symtable, Type *out_type) {
    if (name == NULL || symtable == NULL || out_type == NULL) {
        return ERR_INTERNAL;
    }

    *out_type = TYPE_UNKNOWN;

    VarInfo *var = symtable_find_var(symtable, name);

    if (var == NULL) {
        return ERR_SEM_UNDEF;
    }

    if (var->type == TYPE_UNKNOWN) {
        return ERR_SEM_TYPES;
    }

    *out_type = var->type;

    gen_push_var(name, var->scope_depth);

    return ERR_OK;
}

static ErrorCode parse_identifier(Token *current_token, SymTable *symtable, Type *out_type) {
    if (current_token == NULL || symtable == NULL || out_type == NULL ||
        current_token->value.str_value.data == NULL) {
        return ERR_INTERNAL;
    }

    *out_type = TYPE_UNKNOWN;

    string_t name;
    if (!str_init(&name)) {
        return ERR_INTERNAL;
    }

    if (!str_append_str(&name, current_token->value.str_value.data)) {
        str_free(&name);
        return ERR_INTERNAL;
    }

    token_free(current_token);
    ErrorCode status = get_next_token(current_token);

    if (status == ERR_OK) {
        if (current_token->type == TOKEN_LEFTPAR || current_token->type == TOKEN_LIT_UNIT) {
            status = parse_call(name.data, current_token, symtable, out_type);
        } else {
            status = parse_variable(name.data, symtable, out_type);
        }
    }

    str_free(&name);
    return status;
}

static ErrorCode parse_expression_inner(Token *current_token, SymTable *symtable,
                                        Type *result_type) {
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
    // last shifted input symbol, decides between unary and binary minus
    PrecSymbol prev_input = PREC_SYM_DOLLAR;

    while (true) {
        PrecStackItem *top_term = stack_top_terminal(stack);
        if (top_term == NULL) {
            stack_free(stack);
            return ERR_INTERNAL;
        }

        PrecSymbol input_sym = PREC_SYM_DOLLAR;
        Type input_type = TYPE_UNKNOWN;
        status = token_to_prec_sym(current_token, &input_sym, &input_type);
        if (status != ERR_OK) {
            stack_free(stack);
            return status;
        }

        // minus is binary only after an operand or )
        if (input_sym == PREC_SYM_MINUS && prev_input != PREC_SYM_ID &&
            prev_input != PREC_SYM_RPAR) {
            input_sym = PREC_SYM_UMINUS;
        }

        // ) ends function call argument
        if (top_term->symbol == PREC_SYM_DOLLAR &&
            (input_sym == PREC_SYM_DOLLAR || input_sym == PREC_SYM_RPAR)) {
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

            // variable or function call
            if (input_sym == PREC_SYM_ID && current_token->type == TOKEN_IDENTIFIER) {
                status = parse_identifier(current_token, symtable, &input_type);
                if (status != ERR_OK) {
                    stack_free(stack);
                    return status;
                }

                if (!stack_push(stack, PREC_SYM_ID, input_type)) {
                    stack_free(stack);
                    return ERR_INTERNAL;
                }
                prev_input = PREC_SYM_ID;

                break;
            }

            if (input_sym == PREC_SYM_ID) {
                gen_push_literal(current_token);
            }

            // left operand is already generated, result decides if right operand is skipped
            if (input_sym == PREC_SYM_AND || input_sym == PREC_SYM_OR) {
                int label_id = gen_new_label_id();
                if (!logic_label_push(label_id)) {
                    stack_free(stack);
                    return ERR_INTERNAL;
                }
                gen_logic_start(input_sym, label_id);
            }

            // add input symbol to top
            if (!stack_push(stack, input_sym, input_type)) {
                stack_free(stack);
                return ERR_INTERNAL;
            }
            prev_input = input_sym;

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
            prev_input = input_sym;

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

ErrorCode parse_expression(Token *current_token, SymTable *symtable, Type *result_type) {
    LogicLabel *labels_bottom = logic_labels;

    ErrorCode status = parse_expression_inner(current_token, symtable, result_type);

    // after error labels of unreduced && and || stay on stack
    while (logic_labels != labels_bottom && logic_label_pop(NULL)) {
    }

    return status;
}
