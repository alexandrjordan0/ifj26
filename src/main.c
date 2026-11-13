// File: main.c
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "scanner.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    scanner_init();
    if (parse_program() == ERR_OK) {
        return EXIT_SUCCESS;
    }

    printf(".IFJcode26\n");

    return EXIT_SUCCESS;
}
