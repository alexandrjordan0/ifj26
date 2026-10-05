// File: error.h
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include <stdio.h>

// Error codes
typedef enum {

    ERR_OK = 0,
    ERR_LEXICAL = 1,
    ERR_SYNTAX = 2,
    ERR_SEM_UNDEF = 3,
    ERR_SEM_REDEF = 4,
    ERR_SEM_ARGS = 5,
    ERR_SEM_TYPES = 6,
    ERR_SEM_ASSIGN = 7,
    ERR_SEM_OTHERS = 10,
    ERR_INTERNAL = 99

} ErrorCode;

/**
 * @brief Writes error message on stderr and terminates program with given error code
 * @details Supports variable arguments
 * @param code Error code of translator
 * @param format Error message
 * @param ... Variable arguments
 */
void error_exit(ErrorCode code, const char *format, ...);

#endif // ERROR_H