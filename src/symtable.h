// File: symtable.h
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "error.h"
#include <stddef.h>

/** Types known during semantic analysis*/
typedef enum {
    TYPE_UNKNOWN, // return type of function before return type is known
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_BOOL // extension boolthen
} Type;

/**Information about variable or function parameter
 */
typedef struct {
    Type type;
    bool is_mutable; // var => true; val, function parameter => false
} VarInfo;

/**Information about function */
typedef struct {
    const char *name;
    size_t param_count;
    Type *param_types; // array of param_count Types, NULL if param_count == 0
    const char **param_names;
    Type return_type;
} FuncInfo;

typedef struct SymTable SymTable;

/**
 * @brief create symbol table
 * @return pointer to table, in case of allocation error terminates using error_exit(ERR_INTERNAL)
 */
SymTable *symtable_init();

/**
 * @brief delete symbol table, free everything that symtable allocated itself
 * @param st pointer to symtable
 */
void symtable_free(SymTable *st);

/**
 * @brief add variable to symbol table
 * @param st symbol table
 * @param name name of variable, serves as key, must not be TYPE_UNKNOWN
 * @param type type of variable
 * @param is_mutable true if var; false if var/function parameter
 * @return ERR_OK if ok, ERR_SEM_REDEF if variable already exists, ERR_INTERNAL if reallocation
 * failure
 */
ErrorCode symtable_add_var(SymTable *st, const char *name, Type type, bool is_mutable);

/**
 * @brief return value from given key
 * @param st pointer to symbol table
 * @param name serves as key
 * @return pointer to value, NULL if not found
 */
VarInfo *symtable_find_var(SymTable *st, const char *name);

#endif