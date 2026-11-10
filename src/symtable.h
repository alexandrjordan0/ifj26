// File: symtable.h
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "error.h"
#include <stdbool.h>
#include <stddef.h>

/** Types known during semantic analysis*/
typedef enum {
    TYPE_UNKNOWN, // return type of function before return type is known
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_UNIT,
    TYPE_ANY,
    TYPE_BOOL // extension boolthen
} Type;

/**Information about variable or function parameter
 */
typedef struct {
    Type type;
    bool is_mutable; // var => true; val, function parameter => false
    int scope_depth;
} VarInfo;

/**Information about function */
typedef struct {
    const char *name;
    size_t param_count;
    Type *param_types; // array of param_count Types, NULL if param_count == 0
    const char *const *param_names;
    Type return_type;
    bool is_defined; // true if function is already fully implemented, false otherwise
} FuncInfo;

typedef struct SymTable SymTable;

/**
 * @brief create symbol table
 * @return pointer to table, NULL if allocation failure
 */
SymTable *symtable_init(void);

/**
 * @brief delete symbol table, free everything that symtable allocated itself
 * @param st pointer to symtable
 */
void symtable_free(SymTable *st);

/**
 * @brief create new variable scope
 */
bool symtable_scope_push(SymTable *st);

/**
 * @brief terminate deepest scope
 */
void symtable_scope_pop(SymTable *st);

/**
 * @brief add variable to symbol table into deepest scope
 * @param st symbol table
 * @param name name of variable, serves as key
 * @param type type of variable, must not be TYPE_UNKNOWN
 * @param is_mutable true if var; false if val/function parameter
 * @return ERR_OK if ok, ERR_SEM_REDEF if variable already exists, ERR_INTERNAL if allocation
 * failure
 */
ErrorCode symtable_add_var(SymTable *st, const char *name, Type type, bool is_mutable);

/**
 * @brief find variable from deepest scope first
 * @param st pointer to symbol table
 * @param name serves as key
 * @return pointer to value, NULL if not found
 */
VarInfo *symtable_find_var(SymTable *st, const char *name);
/**
 * @brief find variable in deepest scope
 * @param st pointer to symtable
 * @param name serves as key
 * @return pointer to value, NULL if is not declared in deepest scope
 */
VarInfo *symtable_find_var_current_scope(SymTable *st, const char *name);

/**
 * @brief add function to symbol table. Name, parameter names and types are copied
 * @param return_type TYPE_UNKNOWN if type is not specified
 * @return ERR_OK if ok, ERR_SEM_REDEF if function already exists, ERR_INTERNAL if allocation
 * failure
 */
ErrorCode symtable_add_func(SymTable *st, const char *name, size_t param_count,
                            const Type *param_types, const char *const *param_names,
                            Type return_type, bool is_defined);

/**
 * @brief find function
 * @return pointer to value, NULL if not found
 */
FuncInfo *symtable_find_func(SymTable *st, const char *name);

#endif