// File: symtable.c
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#include "symtable.h"
#include "error.h"
#include "hashtable.h"
#include <stdlib.h>

#define FUNCS_CAPACITY 10
#define SCOPE_CAPACITY 10

typedef struct Scope Scope;

struct Scope {
    Htable *variables;
    Scope *parent;
    int depth;
};

struct SymTable {
    Htable *functions;
    Scope *top;
};

static void func_free(void *ptr) {
    FuncInfo *p = (FuncInfo *)ptr;
    char **names = (char **)p->param_names;
    free((char *)p->name);
    for (size_t i = 0; i < p->param_count; i++) {
        free(names[i]);
    }
    free(names);
    free(p->param_types);
    free(p);
}

SymTable *symtable_init(void) {
    SymTable *new_symtable = malloc(sizeof(SymTable));
    if (new_symtable == NULL) {
        return NULL;
    }
    Htable *funcs = hashtable_init(FUNCS_CAPACITY);
    if (funcs == NULL) {
        free(new_symtable);
        return NULL;
    }
    new_symtable->functions = funcs;
    new_symtable->top = NULL;
    return new_symtable;
}

void symtable_free(SymTable *st) {
    if (st != NULL) {
        while (st->top != NULL) {
            symtable_scope_pop(st);
        }
        hashtable_delete(st->functions, func_free);
        free(st);
    }
}

bool symtable_scope_push(SymTable *st) {
    Scope *new_scope = malloc(sizeof(Scope));
    if (new_scope == NULL) {
        return false;
    }
    new_scope->variables = hashtable_init(SCOPE_CAPACITY);
    if (new_scope->variables == NULL) {
        free(new_scope);
        return false;
    }
    new_scope->parent = st->top;
    new_scope->depth = st->top ? st->top->depth + 1 : 0;
    st->top = new_scope;
    return true;
}

void symtable_scope_pop(SymTable *st) {
    if (st->top == NULL) {
        return;
    }
    Scope *old = st->top;
    st->top = old->parent;
    hashtable_delete(old->variables, free);
    free(old);
}