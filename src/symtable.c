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
    Htable *vars;
    Scope *parent;
    int depth;
};
struct SymTable {
    Htable *funcs;
    Scope *top;
};
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
    new_symtable->funcs = funcs;
    new_symtable->top = NULL;
    return new_symtable;
}