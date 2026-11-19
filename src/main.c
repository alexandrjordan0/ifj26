// File: main.c
// Project: IFJ26
// Authors: Alexandr Jordán (xjordaa00)
// Translator: gcc 16.2.1

#include "parser.h"
#include "scanner.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    scanner_init();

    printf(".IFJcode26\n");

    return parse_program();
}
