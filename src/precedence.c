/**
 * File: precedence.c
 * Project: IFJ26
 * Authors: Alexandr Jordán (xjordaa00)
 * Translator: gcc 16.2.1
 */

#include "precedence.h"

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
