// File: symtable.h
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0
#ifndef SYMTABLE_H
#define SYMTABLE_H

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
    Type *param_types;        // array of param_count Types, NULL if param_count == 0
    const char **param_names; 
    Type return_type;
} FuncInfo;

#endif