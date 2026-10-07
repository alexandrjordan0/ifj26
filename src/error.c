// File: error.c
// Project: IFJ26
// Authors: Michal Jáger (xjagerm00)
// Translator: gcc 14.2.0

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error_exit(ErrorCode code, const char *format, ...) {

    if (format != NULL) {
        fprintf(stderr, "Error [%d]: ", code);
        va_list arguments;
        va_start(arguments, format);
        vfprintf(stderr, format, arguments);
        va_end(arguments);
        fprintf(stderr, "\n");
    }

    exit(code);
}