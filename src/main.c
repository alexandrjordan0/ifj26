// File: main.c
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#include "codegenerator.h"
#include "error.h"
#include "parser.h"
#include "scanner.h"
#include <stdlib.h>

// TODO: parser does not expose name of main function, change when parser provides it
#define MAIN_FUNC_NAME "main"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    scanner_init();
    gen_init();

    // TODO: parser does not call gen_* functions yet, generated code contains only
    // header and builtins until it is connected
    ErrorCode err = parse_program();
    if (err != ERR_OK) {
        gen_free();
        error_exit(err, "compilation failed");
    }

    gen_finish(MAIN_FUNC_NAME);
    gen_free();

    return EXIT_SUCCESS;
}
