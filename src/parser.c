/**
 * File: parser.c
 * Project: IFJ26
 * Authors: Matyáš Filip Jurásek (xjurasm00)
 * Translator: gcc 16.2.1
 */

#include "parser.h"
#include "error.h"
// #include "scanner.h"

ErrorCode parse_program() {
    /*
     * TODO
     */
    if (parse_prolog() != ERR_OK) {
        return ERR_SYNTAX;
    }
    if (parse_func_set() != ERR_OK) {
        return ERR_SYNTAX;
    }
    /* if (get_next_token(&token) != TOKEN_EOF) {
     *  return ERR_SYNTAX;
     * }
     */
    return ERR_OK;
}

ErrorCode parse_prolog() {
    /**
     * if (get_next_token(&token) != TOKEN_IMPORT) {
     *  return ERR_SYNTAX
     * }
     * if (get_next_token(&token) != TOKEN_ID) {
     *  return ERR_SYNTAX
     * }
     * if (get_next_token(&token) != TOKEN_DOT) {
     *  return ERR_SYNTAX
     * }
     * if (get_next_token(&token) != TOKEN_STAR) {
     *  return ERR_SYNTAX
     * }
     * if (get_next_token(&token) != TOKEN_EOL) {
     *  return ERR_SYNTAX
     * }
     */
    return ERR_OK;
}

ErrorCode parse_func_set() {
    /**
     * TODO
     */
    return ERR_OK;
}
