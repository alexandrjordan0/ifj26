// File: scanner.c
// Project: IFJ26
// Authors: Patrik Lisál (xlisalp00)

#include "scanner.h"
#include "str.h"

#include <errno.h>
#include <math.h>
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
static int paren_depth = 0; // parentehsis nesting

static int look_ahead[2];
static int la_count = 0;

void scanner_init(void) {
    line = 1;
    col = 0;
    at_line_start = true;

    indent_top = 0;
    memset(indent_stack, 0, sizeof indent_stack);
    indent_stack[0] = 0;

    pending_dedents = 0;
    pending_indent = false;

    paren_depth = 0;

    look_ahead[0] = EOF;
    look_ahead[1] = EOF;
    la_count = 0;
}

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

static int next_char(void) {
    fill_lookahead(1);
    int c = look_ahead[0];
    look_ahead[0] = look_ahead[1];
    la_count--;

    if (c == '\n') {
        line++;
        col = 0;
    } else if (c != EOF && c != '\r') {
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
    for (;;) {
        int spaces = 0;
        bool has_tab = false;

        while (peek_char() == ' ') {
            next_char();
            spaces++;
        }

        for (;;) {
            int c = peek_char();

            // Empty line
            if (c == '\n') {
                next_char();
                goto next_line;
            }

            if (c == '\r' && peek_char2() == '\n') {
                next_char();
                next_char();
                goto next_line;
            }

            if (c == EOF) {
                return ERR_OK;
            }

            // Whitespace after indentation or a comment
            if (c == ' ' || c == '\r') {
                next_char();
                continue;
            }

            if (c == '\t') {
                has_tab = true;
                next_char();
                continue;
            }

            // Ignore a line comment
            if (c == '/' && peek_char2() == '/') {
                while (peek_char() != '\n' && peek_char() != EOF) {
                    next_char();
                }

                if (peek_char() == '\n') {
                    next_char();
                }

                goto next_line;
            }

            // Handle a block comment at the beginning of a line
            if (c == '/' && peek_char2() == '*') {
                int depth = 1;
                bool has_newline = false;

                next_char();
                next_char();

                while (depth > 0) {
                    c = peek_char();

                    if (c == EOF) {
                        return ERR_LEXICAL;
                    }

                    if (c == '/' && peek_char2() == '*') {
                        next_char();
                        next_char();
                        depth++;
                    } else if (c == '*' && peek_char2() == '/') {
                        next_char();
                        next_char();
                        depth--;
                    } else {
                        if (c == '\n') {
                            has_newline = true;
                        }

                        next_char();
                    }
                }

                // A multiline block comment acts as an EOL.
                if (has_newline) {
                    while (peek_char() == ' ' || peek_char() == '\t' || peek_char() == '\r') {
                        next_char();
                    }

                    c = peek_char();

                    if (c != '\n' && c != EOF) {
                        return ERR_LEXICAL;
                    }

                    if (c == '\n') {
                        next_char();
                    }

                    goto next_line;
                }

                // Single-line comment: check what follows it.
                continue;
            }

            break;
        }

        // Tabs are forbidden before code on a non-empty line.
        if (has_tab) {
            return ERR_LEXICAL;
        }

        // Compare indentation with the previous level.
        if (spaces > indent_stack[indent_top]) {
            if (indent_top + 1 >= INDENT_STACK_MAX) {
                return ERR_INTERNAL;
            }

            indent_stack[++indent_top] = spaces;
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

    next_line:
        continue;
    }
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

static void remove_number_underscores(string_t *buf) {
    size_t write = 0;

    for (size_t read = 0; read < buf->length; read++) {
        if (buf->data[read] != '_') {
            buf->data[write++] = buf->data[read];
        }
    }

    buf->data[write] = '\0';
    buf->length = write;
}

static ErrorCode finish_int(Token *token, string_t *buf) {
    remove_number_underscores(buf);

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
    remove_number_underscores(buf);

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
    if (pending_dedents > 0) {
        pending_dedents--;
        token->type = TOKEN_DEDENT;
        token->line = line;
        token->col = col + 1;
        return ERR_OK;
    }

    if (pending_indent) {
        pending_indent = false;
        token->type = TOKEN_INDENT;
        token->line = line;
        token->col = col + 1;
        return ERR_OK;
    }

    ErrorCode err;
    State state = S_START;
    string_t buf;
    int depth = 0;
    bool block_comment_has_new_line = false;

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
                if (paren_depth > 0) {
                    at_line_start = false;
                    continue;
                }
                return emit(token, &buf, TOKEN_EOL);
            } else if (c == '0') {
                if (!str_append_char(&buf, c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_ZERO;
            } else if (is_digit(c)) {
                if (!str_append_char(&buf, c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_INT;
            } else if (is_ident_start(c)) {
                if (!str_append_char(&buf, c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
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
                if (paren_depth > 0) {
                    paren_depth--;
                }
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
                if (is_digit(peek_char())) {

                    if (!str_append_char(&buf, '.')) {
                        str_free(&buf);
                        return ERR_INTERNAL;
                    }

                    state = S_INT_DOT;
                } else {
                    return emit(token, &buf, TOKEN_DOT);
                }
            } else {
                return lex_error(&buf); // unknown symbol
            }

            break;

        case S_IDENT:
            if (is_ident_char(c)) {
                next_char();
                if (!str_append_char(&buf, c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
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
                if (!str_append_char(&buf, '.')) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_INT_DOT;
            } else if (c == 'e' || c == 'E') {
                next_char();
                if (!str_append_char(&buf, ((char)c))) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_EXP;
            } else if (c == 'd' || c == 'D') {
                next_char();
                if (!str_append_char(&buf, ((char)c))) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
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
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            } else if (c == '_') {
                if (!is_digit(peek_char2()))
                    return lex_error(&buf);
                next_char();
                if (!str_append_char(&buf, '_')) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_INT_UNDERSCORE;
            } else if (c == '.') {
                next_char();
                if (!str_append_char(&buf, '.')) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_INT_DOT;
            } else if (c == 'e' || c == 'E') {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_EXP;
            } else if (c == 'd' || c == 'D') {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
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
            if (!str_append_char(&buf, (char)c)) {
                str_free(&buf);
                return ERR_INTERNAL;
            }
            state = S_INT;

            break;

        case S_INT_DOT:
            if (!is_digit(c))
                return lex_error(&buf); // digits are required after '.'
            next_char();
            if (!str_append_char(&buf, (char)c)) {
                str_free(&buf);
                return ERR_INTERNAL;
            }
            state = S_DOUBLE;

            break;

        case S_DOUBLE:
            if (is_digit(c)) {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            } else if (c == '_') {
                if (!is_digit(peek_char2()))
                    return lex_error(&buf);
                next_char();
                if (!str_append_char(&buf, '_')) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            } else if (c == 'e' || c == 'E') {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_EXP;
            } else if (c == 'd' || c == 'D') {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                return finish_double(token, &buf);
            } else if (is_alpha(c)) {
                return lex_error(&buf);
            } else {
                return finish_double(token, &buf);
            }

            break;

        case S_EXP:
            if (c == '+' || c == '-') {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_EXP_SIGN;
            } else if (is_digit(c)) {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                state = S_EXP_DIGITS;
            } else {
                return lex_error(&buf);
            }

            break;

        case S_EXP_SIGN:
            if (!is_digit(c))
                return lex_error(&buf);
            next_char();
            if (!str_append_char(&buf, (char)c)) {
                str_free(&buf);
                return ERR_INTERNAL;
            }
            state = S_EXP_DIGITS;

            break;

        case S_EXP_DIGITS:
            if (is_digit(c)) {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            } else if (c == '_') {
                if (!is_digit(peek_char2()))
                    return lex_error(&buf);
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            } else if (c == 'd' || c == 'D') {
                next_char();
                if (!str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
                return finish_double(token, &buf);
            } else if (is_alpha(c)) {
                return lex_error(&buf);
            } else {
                return finish_double(token, &buf);
            }
            break;

        case S_QUOTE1:
            if (c == '"') {
                next_char();
                state = S_QUOTE2;
            } else {
                state = S_STRING;
            }

            break;

        case S_QUOTE2:
            if (c == '"') {
                next_char();
                state = S_MLSTRING;
            } else {
                return finish_string(token, &buf);
            }

            break;

        case S_STRING:
            if (c == EOF || c == '\n' || c == '\r') {
                return lex_error(&buf);
            }
            if (c == '"') {
                next_char();
                return finish_string(token, &buf);
            }
            if (c == '\\') {
                next_char();
                state = S_STRING_ESC;
                break;
            }
            next_char();
            if (!str_append_char(&buf, (char)c)) {
                str_free(&buf);
                return ERR_INTERNAL;
            }

            break;

        case S_STRING_ESC: {
            char escaped;
            if (c == EOF) {
                return lex_error(&buf);
            }
            next_char();
            switch (c) {
            case '\\':
                escaped = '\\';
                break;
            case '"':
                escaped = '"';
                break;
            case 'n':
                escaped = '\n';
                break;
            case 'r':
                escaped = '\r';
                break;
            case 't':
                escaped = '\t';
                break;
            default:
                return lex_error(&buf);
            }
            if (!str_append_char(&buf, escaped)) {
                str_free(&buf);
                return ERR_INTERNAL;
            }
            state = S_STRING;

            break;
        }

        case S_MLSTRING:
            if (c == EOF) {
                return lex_error(&buf);
            }
            if (c == '"') {
                next_char();
                if (peek_char() == '"' && peek_char2() == '"') {
                    next_char();
                    next_char();
                    return finish_string(token, &buf);
                }
                if (!str_append_char(&buf, '"')) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            } else {
                next_char();
                if (c != '\r' && !str_append_char(&buf, (char)c)) {
                    str_free(&buf);
                    return ERR_INTERNAL;
                }
            }
            break;

        case S_SLASH:
            if (c == '/') {
                next_char();
                state = S_LINE_COMMENT;
            } else if (c == '*') {
                next_char();
                depth = 1;
                block_comment_has_new_line = false;
                state = S_BLOCK_COMMENT;
            } else {
                return emit(token, &buf, TOKEN_DIV);
            }

            break;

        case S_LINE_COMMENT:
            if (c == EOF || c == '\n') {
                state = S_START;
            } else {
                next_char();
            }

            break;

        case S_BLOCK_COMMENT:
            if (c == EOF) {
                return lex_error(&buf);
            }
            if (c == '/' && peek_char2() == '*') {
                next_char();
                next_char();
                depth++;
            } else if (c == '*' && peek_char2() == '/') {
                next_char();
                next_char();
                depth--;

                if (depth == 0) {
                    if (!block_comment_has_new_line) {
                        state = S_START;
                    } else {
                        /* after a multiline comment, only whitespace
                           may appear before the physical newline */
                        while (peek_char() == ' ' || peek_char() == '\t' || peek_char() == '\r') {
                            next_char();
                        }

                        c = peek_char();

                        if (c != '\n' && c != EOF) {
                            return lex_error(&buf);
                        }

                        token->line = line;
                        token->col = col + 1;

                        if (c == '\n') {
                            next_char();
                        }

                        at_line_start = true;
                        return emit(token, &buf, TOKEN_EOL);
                    }
                }
            } else {
                if (c == '\n') {
                    block_comment_has_new_line = true;
                }

                next_char();
            }
            break;

        case S_LT:
            if (c == '=') {
                next_char();
                return emit(token, &buf, TOKEN_LESSOREQ);
            }
            return emit(token, &buf, TOKEN_LESSTHAN);

            break;

        case S_GT:
            if (c == '=') {
                next_char();
                return emit(token, &buf, TOKEN_GREATEROREQ);
            }
            return emit(token, &buf, TOKEN_GREATERTHAN);

            break;

        case S_ASSIGN:
            if (c == '=') {
                next_char();
                return emit(token, &buf, TOKEN_EQ);
            }
            return emit(token, &buf, TOKEN_ASSIGN);

            break;

        case S_NOT:
            if (c == '=') {
                next_char();
                return emit(token, &buf, TOKEN_NEQ);
            }
            return emit(token, &buf, TOKEN_NOT);

            break;

        case S_AND:
            if (c != '&')
                return lex_error(&buf);
            next_char();
            return emit(token, &buf, TOKEN_AND);

            break;

        case S_OR:
            if (c != '|')
                return lex_error(&buf);
            next_char();
            return emit(token, &buf, TOKEN_OR);

            break;

        case S_LPAR:
            if (c == ')') {
                next_char();
                return emit(token, &buf, TOKEN_LIT_UNIT);
            }
            paren_depth++;
            return emit(token, &buf, TOKEN_LEFTPAR);

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