// File: main.c
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#include "codegenerator.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    scanner_init();
    gen_init();

    ErrorCode err = parse_program();
    gen_free();
    if (err != ERR_OK) {
        error_exit(err, "compilation failed");
    }

    return EXIT_SUCCESS;
}
