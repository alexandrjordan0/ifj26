// File: scanner.c
// Project: IFJ26
// Authors: Patrik Lisál (xlisalp00)

#include "scanner.h"
#include "str.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int line = 1;
static int col = 0;
static bool at_line_start = true;

#define INDENT_STACK_MAX 256
static int indent_top = 0; // index of the top of the stack
static int indent_stack[INDENT_STACK_MAX] = {0};
static int pending_dedents = 0; // amount of DEDENT tokens to be emited
static bool pending_indent = false;

static int look_ahead[2];
static int la_count = 0;

static void fill_lookahead(int n) {
    while (la_count < n) {
        look_ahead[la_count++] = getc(stdin);
    }
}

static int peek_char(void) {
    fill_lookahead(1);
    return look_ahead[0];
}

static int peek_char2(void) {
    fill_lookahead(2);
    return look_ahead[1];
}

int next_char(void) {
    fill_lookahead(1);
    int c = look_ahead[0];
    look_ahead[0] = look_ahead[1];
    la_count--;

    if (c == '\n') {
        line++;
        col = 0;
    } else if (c != EOF) {
        col++;
    }
    return c;
}

static bool is_digit(int c) { return c >= '0' && c <= '9'; }
static bool is_alpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool is_ident_start(int c) { return is_alpha(c) || c == '_'; }
static bool is_ident_char(int c) { return is_alpha(c) || is_digit(c) || c == '_'; }
/*------------------------*/
/* STATES OF AUTOMATA     */
/*------------------------*/

typedef enum {
    S_START,

    S_IDENT,

    S_ZERO, // "0" at beginning of int
    S_INT,
    S_INT_UNDERSCORE,
    S_INT_DOT,
    S_DOUBLE,
    S_EXP,
    S_EXP_SIGN,
    S_EXP_DIGITS,

    S_QUOTE1, // saw "
    S_QUOTE2, // saw ""
    S_STRING,
    S_STRING_ESC,
    S_MLSTRING,

    S_SLASH,
    S_LINE_COMMENT,
    S_BLOCK_COMMENT,

    S_LT,
    S_GT,
    S_ASSIGN,
    S_NOT,
    S_AND,
    S_OR,
    S_LPAR
} State;

/*------------------------*/
/* KEYWORD TABLE          */
/*------------------------*/

static const struct {
    const char *name;
    Token_type type;
} keywords[] = {
    {"def", TOKEN_KEYWORD_DEF},
    {"do", TOKEN_KEYWORD_DO},
    {"Double", TOKEN_KEYWORD_DOUBLE},
    {"if", TOKEN_KEYWORD_IF},
    {"else", TOKEN_KEYWORD_ELSE},
    {"import", TOKEN_KEYWORD_IMPORT},
    {"Int", TOKEN_KEYWORD_INT},
    {"return", TOKEN_KEYWORD_RETURN},
    {"String", TOKEN_KEYWORD_STRING},
    {"then", TOKEN_KEYWORD_THEN},
    {"Unit", TOKEN_KEYWORD_UNIT},
    {"val", TOKEN_KEYWORD_VAL},
    {"var", TOKEN_KEYWORD_VAR},
    {"while", TOKEN_KEYWORD_WHILE},
    // BOOLTHEN
    {"Boolean", TOKEN_KEYWORD_BOOLEAN},
    {"true", TOKEN_KEYWORD_TRUE},
    {"false", TOKEN_KEYWORD_FALSE},
};

static Token_type keyword_or_ident(const char *s) {
    for (size_t i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(keywords[i].name, s) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}

static ErrorCode handle_indentation(void) {
    int spaces = 0;
    while (peek_char() == ' ') {
        next_char();
        spaces++;
    }

    int c = peek_char();

    if (c == '\t') {
        // tab error
        while (peek_char() == ' ' || peek_char() == '\t' || peek_char() == '\r') {
            next_char();
        }
        c = peek_char();
        if (c == '\n' || c == EOF) {
            return ERR_OK;
        }
        return ERR_LEXICAL;
    }

    bool blank = (c == '\n' || c == EOF || (c == '\r' && peek_char2() == '\n'));
    bool comment_only = (c == '/' && peek_char2() == '/');
    if (blank || comment_only) {
        return ERR_OK;
    }

    if (spaces > indent_stack[indent_top]) {
        if (indent_top + 1 >= INDENT_STACK_MAX) {
            return ERR_INTERNAL;
        }
        indent_stack[indent_top++] = spaces;
        pending_indent = true;
    } else {
        while (spaces < indent_stack[indent_top]) {
            indent_top--;
            pending_dedents++;
        }
        if (spaces != indent_stack[indent_top]) {
            return ERR_LEXICAL;
        }
    }
    return ERR_OK;
}

static ErrorCode emit(Token *token, string_t *buf, Token_type type) {
    token->type = type;
    str_free(buf);
    return ERR_OK;
}

static ErrorCode lex_error(string_t *buf) {
    str_free(buf);
    return ERR_LEXICAL;
}

static ErrorCode finish_int(Token *token, string_t *buf) {
    errno = 0;
    long long v = strtoll(buf->data, NULL, 10);
    bool overflow = (errno == ERANGE);
    if (overflow) {
        return lex_error(buf);
    }
    token->value.int_value = v;
    return emit(token, buf, TOKEN_LIT_INT);
}

static ErrorCode finish_double(Token *token, string_t *buf) {
    errno = 0;
    double v = strtod(buf->data, NULL);
    bool overflow = (errno == ERANGE && isinf(v));
    if (overflow) {
        return lex_error(buf);
    }
    token->value.double_value = v;
    return emit(token, buf, TOKEN_LIT_DOUBLE);
}

static ErrorCode finish_string(Token *token, string_t *buf) {
    token->type = TOKEN_LIT_STRING;
    token->value.str_value = *buf;
    return ERR_OK;
}
/*------------------------*/
/* MAIN FUNCTION          */
/*------------------------*/
ErrorCode get_next_token(Token *token) {
    ErrorCode err;
    State state = S_START;
    string_t buf;
    int quotes = 0;
    int depth = 0;

    memset(token, 0, sizeof *token);
    token->line = line;
    token->col = col;

    // indentaion at the beginning of a line
    if (at_line_start) {
        at_line_start = false;
        err = handle_indentation();
        if (err != ERR_OK) {
            return err;
        }
    }

    // waiting indent/dedent
    if (pending_dedents > 0) {
        pending_dedents--;
        token->type = TOKEN_DEDENT;
        return ERR_OK;
    }
    if (pending_indent) {
        pending_indent = false;
        token->type = TOKEN_INDENT;
        return ERR_OK;
    }

    if (!str_init(&buf)) {
        return ERR_INTERNAL;
    }

    for (;;) {
        int c = peek_char();

        switch (state) {

        case S_START:
            token->line = line;
            token->col = col + 1;

            if (c == EOF) {
                if (indent_top > 0) {
                    indent_top--;
                    return emit(token, &buf, TOKEN_DEDENT);
                }
                return emit(token, &buf, TOKEN_EOF);
            }

            next_char();
            if (c == ' ' || c == '\t' || c == '\r') {
                // skip whitespaces in line
            } else if (c == '\n') {
                at_line_start = true;
                return emit(token, &buf, TOKEN_EOL);
            } else if (c == '0') {
                str_append_char(&buf, c);
                state = S_ZERO;
            } else if (is_digit(c)) {
                str_append_char(&buf, c);
                state = S_INT;
            } else if (is_ident_start(c)) {
                str_append_char(&buf, c);
                state = S_IDENT;
            } else if (c == '"') {
                state = S_QUOTE1;
            } else if (c == '/') {
                state = S_SLASH;
            } else if (c == '<') {
                state = S_LT;
            } else if (c == '>') {
                state = S_GT;
            } else if (c == '=') {
                state = S_ASSIGN;
            } else if (c == '!') {
                state = S_NOT;
            } else if (c == '&') {
                state = S_AND;
            } else if (c == '|') {
                state = S_OR;
            } else if (c == '(') {
                state = S_LPAR;
            } else if (c == '+') {
                return emit(token, &buf, TOKEN_PLUS);
            } else if (c == '-') {
                return emit(token, &buf, TOKEN_MINUS);
            } else if (c == '*') {
                return emit(token, &buf, TOKEN_MUL);
            } else if (c == ')') {
                return emit(token, &buf, TOKEN_RIGHTPAR);
            } else if (c == ',') {
                return emit(token, &buf, TOKEN_COMMA);
            } else if (c == ':') {
                return emit(token, &buf, TOKEN_COLON);
            } else if (c == ';') {
                return emit(token, &buf, TOKEN_SEMICOLON);
            } else if (c == '@') {
                return emit(token, &buf, TOKEN_AT);
            } else if (c == '.') {
                return emit(token, &buf, TOKEN_DOT);
            } else {
                return lex_error(&buf); // unknown symbol
            }

            break;

        case S_IDENT:
            if (is_ident_char(c)) {
                next_char();
                str_append_char(&buf, c);
                break;
            }
            // ident ended
            if (strcmp(buf.data, "_") == 0) {
                return emit(token, &buf, TOKEN_UNDERSCORE);
            }
            token->type = keyword_or_ident(buf.data);
            if (token->type == TOKEN_IDENTIFIER) {
                token->value.str_value = buf;
                return ERR_OK;
            }
            str_free(&buf);
            return ERR_OK;

            break;

        case S_ZERO:
            if (is_digit(c) || c == '_') {
                return lex_error(&buf); // no leaading zeroes in integer lits
            }
            if (c == '.') {
                next_char();
                str_append_char(&buf, '.');
                state = S_INT_DOT;
            } else if (c == 'e' || c == 'E') {
                next_char();
                str_append_char(&buf, ((char)c));
                state = S_EXP;
            } else if (c == 'd' || c == 'D') {
                next_char();
                str_append_char(&buf, ((char)c));
                return finish_double(token, &buf);
            } else if (is_alpha(c)) {
                return lex_error(&buf);
            } else {
                return finish_int(token, &buf);
            }
            break;

        case S_INT:

            if (is_digit(c)) {
                next_char();
                str_append_char(&buf, (char)c);
            } else if (c == '_') {
                if (!is_digit(peek_char2()))
                    return lex_error(&buf);
                next_char();
                str_append_char(&buf, '_');
                state = S_INT_UNDERSCORE;
            } else if (c == '.') {
                next_char();
                str_append_char(&buf, '.');
                state = S_INT_DOT;
            } else if (c == 'e' || c == 'E') {
                next_char();
                str_append_char(&buf, (char)c);
                state = S_EXP;
            } else if (c == 'd' || c == 'D') {
                next_char();
                str_append_char(&buf, (char)c);
                return finish_double(token, &buf);
            } else if (is_alpha(c)) {
                return lex_error(&buf);
            } else {
                return finish_int(token, &buf);
            }

            break;

        case S_INT_UNDERSCORE:
            if (!is_digit(c))
                return lex_error(&buf);
            next_char();
            str_append_char(&buf, (char)c);
            state = S_INT;

            break;

        case S_INT_DOT:
            if (!is_digit(c))
                return lex_error(&buf); // digits are required after '.'
            next_char();
            str_append_char(&buf, (char)c);
            state = S_DOUBLE;

            break;

        case S_DOUBLE:
            if (is_digit(c)) {
                next_char();
                str_append_char(&buf, (char)c);
            } else if (c == '_') {
                if (!is_digit(peek_char()))
                    return lex_error(&buf);
                next_char();
                str_append_char(&buf, (char)c);
            } else if (c == 'e' || c == 'E') {
                next_char();
                str_append_char(&buf, (char)c);
                state = S_EXP;
            } else if (c == 'd' || c == 'D') {
                next_char();
                str_append_char(&buf, (char)c);
                return finish_double(token, &buf);
            } else if (is_alpha(c)) {
                return lex_error(&buf);
            } else {
                return finish_double(token, &buf);
            }

            break;

        case S_EXP:

            break;

        case S_EXP_SIGN:
            break;

        case S_EXP_DIGITS:
            break;

        case S_QUOTE1:
            break;

        case S_QUOTE2:
            break;

        case S_STRING:
            break;

        case S_STRING_ESC:
            break;

        case S_MLSTRING:
            break;

        case S_SLASH:
            break;

        case S_LINE_COMMENT:
            break;

        case S_BLOCK_COMMENT:
            break;

        case S_LT:
            break;

        case S_GT:
            break;

        case S_ASSIGN:
            break;

        case S_NOT:
            break;

        case S_AND:
            break;

        case S_OR:
            break;

        case S_LPAR:
            break;
        }
    }
}

void token_free(Token *token) {
    if (token == NULL) {
        return;
    }
    if (token->type == TOKEN_IDENTIFIER || token->type == TOKEN_LIT_STRING) {
        str_free(&token->value.str_value);
    }
}