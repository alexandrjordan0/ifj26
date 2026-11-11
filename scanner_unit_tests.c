#define _POSIX_C_SOURCE 200809L

#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int assertions = 0;
static int failed_assertions = 0;

static int set_input(const char *input)
{
    char path[] = "/tmp/ifj26-scanner-test-XXXXXX";
    int fd = mkstemp(path);

    if (fd == -1) {
        perror("mkstemp");
        return 0;
    }

    size_t length = strlen(input);
    size_t written_total = 0;

    while (written_total < length) {
        ssize_t written = write(fd, input + written_total, length - written_total);
        if (written <= 0) {
            perror("write");
            close(fd);
            unlink(path);
            return 0;
        }
        written_total += (size_t)written;
    }

    if (close(fd) != 0) {
        perror("close");
        unlink(path);
        return 0;
    }

    if (freopen(path, "r", stdin) == NULL) {
        perror("freopen");
        unlink(path);
        return 0;
    }
    unlink(path);
    clearerr(stdin);

    /* Reset all scanner state for every independent input. */
    scanner_init();
    return 1;
}

static int next_token(Token *token)
{
    ErrorCode error = get_next_token(token);
    if (error != ERR_OK) {
        fprintf(stderr, "    expected a token, scanner returned error %d\n", error);
        failed_assertions++;
        return 0;
    }
    return 1;
}

static int expect_type(Token_type expected)
{
    Token token;
    if (!next_token(&token)) {
        return 0;
    }

    assertions++;
    if (token.type != expected) {
        fprintf(stderr, "    expected token type %d, got %d at %d:%d\n",
                expected, token.type, token.line, token.col);
        failed_assertions++;
        token_free(&token);
        return 0;
    }

    token_free(&token);
    return 1;
}

static int expect_position(Token_type expected, int line, int col)
{
    Token token;
    if (!next_token(&token)) {
        return 0;
    }

    assertions++;
    if (token.type != expected || token.line != line || token.col != col) {
        fprintf(stderr,
                "    expected type %d at %d:%d, got type %d at %d:%d\n",
                expected, line, col, token.type, token.line, token.col);
        failed_assertions++;
        token_free(&token);
        return 0;
    }

    token_free(&token);
    return 1;
}

static int expect_identifier(const char *expected)
{
    Token token;
    if (!next_token(&token)) {
        return 0;
    }

    assertions++;
    if (token.type != TOKEN_IDENTIFIER || token.value.str_value.data == NULL ||
        strcmp(token.value.str_value.data, expected) != 0) {
        fprintf(stderr, "    expected identifier \"%s\"\n", expected);
        failed_assertions++;
        token_free(&token);
        return 0;
    }

    token_free(&token);
    return 1;
}

static int expect_string(const char *expected)
{
    Token token;
    if (!next_token(&token)) {
        return 0;
    }

    assertions++;
    if (token.type != TOKEN_LIT_STRING || token.value.str_value.data == NULL ||
        strcmp(token.value.str_value.data, expected) != 0) {
        fprintf(stderr, "    expected string literal \"%s\"\n", expected);
        failed_assertions++;
        token_free(&token);
        return 0;
    }

    token_free(&token);
    return 1;
}

static int expect_int(long long expected)
{
    Token token;
    if (!next_token(&token)) {
        return 0;
    }

    assertions++;
    if (token.type != TOKEN_LIT_INT || token.value.int_value != expected) {
        fprintf(stderr, "    expected integer %lld\n", expected);
        failed_assertions++;
        token_free(&token);
        return 0;
    }

    token_free(&token);
    return 1;
}

static int expect_double(double expected)
{
    Token token;
    if (!next_token(&token)) {
        return 0;
    }

    assertions++;
    if (token.type != TOKEN_LIT_DOUBLE || token.value.double_value != expected) {
        fprintf(stderr, "    expected double %.17g\n", expected);
        failed_assertions++;
        token_free(&token);
        return 0;
    }

    token_free(&token);
    return 1;
}

static int expect_lexical_error(const char *label, const char *input)
{
    if (!set_input(input)) {
        fprintf(stderr, "    could not prepare input for: %s\n", label);
        failed_assertions++;
        return 0;
    }

    /* Consume any valid prefix; the test passes only if a lexical error occurs. */
    for (int i = 0; i < 10000; i++) {
        Token token;
        ErrorCode error = get_next_token(&token);

        if (error == ERR_LEXICAL) {
            assertions++;
            return 1;
        }
        if (error != ERR_OK) {
            fprintf(stderr, "    %s: expected ERR_LEXICAL, got error %d\n", label, error);
            failed_assertions++;
            return 0;
        }
        if (token.type == TOKEN_EOF) {
            fprintf(stderr, "    %s: expected ERR_LEXICAL, got TOKEN_EOF\n", label);
            failed_assertions++;
            token_free(&token);
            return 0;
        }
        token_free(&token);
    }

    fprintf(stderr, "    %s: scanner did not terminate\n", label);
    failed_assertions++;
    return 0;
}

#define REQUIRE(expr) do { if (!(expr)) return 0; } while (0)

static int test_keywords_identifiers(void)
{
    REQUIRE(set_input("def defx Def _abc a_b2 true false True _\n"));
    REQUIRE(expect_type(TOKEN_KEYWORD_DEF));
    REQUIRE(expect_identifier("defx"));
    REQUIRE(expect_identifier("Def"));
    REQUIRE(expect_identifier("_abc"));
    REQUIRE(expect_identifier("a_b2"));
    REQUIRE(expect_type(TOKEN_KEYWORD_TRUE));
    REQUIRE(expect_type(TOKEN_KEYWORD_FALSE));
    REQUIRE(expect_identifier("True"));
    REQUIRE(expect_type(TOKEN_UNDERSCORE));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_numbers(void)
{
    REQUIRE(set_input("0 12_345 .5 1.5 1_234.5_6 2.5e1_0 1.2d 2.3D\n"));
    REQUIRE(expect_int(0));
    REQUIRE(expect_int(12345));
    REQUIRE(expect_double(0.5));
    REQUIRE(expect_double(1.5));
    REQUIRE(expect_double(1234.56));
    REQUIRE(expect_double(25000000000.0));
    REQUIRE(expect_double(1.2));
    REQUIRE(expect_double(2.3));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_strings_and_crlf_multiline(void)
{
    const char input[] = "\"hello\\nworld\" \"\"\"\"\"\" \"\"\"a\r\nb\"\"\"\n";
    REQUIRE(set_input(input));
    REQUIRE(expect_string("hello\nworld"));
    REQUIRE(expect_string(""));
    /* Scanner normalizes Windows CRLF inside multiline strings to LF. */
    REQUIRE(expect_string("a\nb"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_operators_and_unit(void)
{
    REQUIRE(set_input("== != <= >= && || ! = < > + - * / , : ; @main . ()\n"));
    REQUIRE(expect_type(TOKEN_EQ));
    REQUIRE(expect_type(TOKEN_NEQ));
    REQUIRE(expect_type(TOKEN_LESSOREQ));
    REQUIRE(expect_type(TOKEN_GREATEROREQ));
    REQUIRE(expect_type(TOKEN_AND));
    REQUIRE(expect_type(TOKEN_OR));
    REQUIRE(expect_type(TOKEN_NOT));
    REQUIRE(expect_type(TOKEN_ASSIGN));
    REQUIRE(expect_type(TOKEN_LESSTHAN));
    REQUIRE(expect_type(TOKEN_GREATERTHAN));
    REQUIRE(expect_type(TOKEN_PLUS));
    REQUIRE(expect_type(TOKEN_MINUS));
    REQUIRE(expect_type(TOKEN_MUL));
    REQUIRE(expect_type(TOKEN_DIV));
    REQUIRE(expect_type(TOKEN_COMMA));
    REQUIRE(expect_type(TOKEN_COLON));
    REQUIRE(expect_type(TOKEN_SEMICOLON));
    REQUIRE(expect_type(TOKEN_MAIN));
    REQUIRE(expect_type(TOKEN_DOT));
    REQUIRE(expect_type(TOKEN_LIT_UNIT));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_indentation_positions(void)
{
    REQUIRE(set_input("a\n    b\n        c\n    d\ne\n"));
    REQUIRE(expect_position(TOKEN_IDENTIFIER, 1, 1));
    REQUIRE(expect_position(TOKEN_EOL, 1, 2));
    REQUIRE(expect_position(TOKEN_INDENT, 2, 5));
    REQUIRE(expect_position(TOKEN_IDENTIFIER, 2, 5));
    REQUIRE(expect_position(TOKEN_EOL, 2, 6));
    REQUIRE(expect_position(TOKEN_INDENT, 3, 9));
    REQUIRE(expect_position(TOKEN_IDENTIFIER, 3, 9));
    REQUIRE(expect_position(TOKEN_EOL, 3, 10));
    REQUIRE(expect_position(TOKEN_DEDENT, 4, 5));
    REQUIRE(expect_position(TOKEN_IDENTIFIER, 4, 5));
    REQUIRE(expect_position(TOKEN_EOL, 4, 6));
    REQUIRE(expect_position(TOKEN_DEDENT, 5, 1));
    REQUIRE(expect_position(TOKEN_IDENTIFIER, 5, 1));
    REQUIRE(expect_position(TOKEN_EOL, 5, 2));
    REQUIRE(expect_position(TOKEN_EOF, 6, 1));
    return 1;
}

static int test_nested_comments_and_comment_lines(void)
{
    REQUIRE(set_input("a /* outer /* inner */ end */ + b\n"
                      "c\n    d\n    // line comment\n    /* block comment */\n    e\nf\n"));
    REQUIRE(expect_identifier("a"));
    REQUIRE(expect_type(TOKEN_PLUS));
    REQUIRE(expect_identifier("b"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_identifier("c"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_INDENT));
    REQUIRE(expect_identifier("d"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_identifier("e"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_DEDENT));
    REQUIRE(expect_identifier("f"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_parentheses_and_eof_dedents(void)
{
    REQUIRE(set_input("foo(\n    bar(\n        a\n    ),\n    b\n)\nc\n"));
    REQUIRE(expect_identifier("foo"));
    REQUIRE(expect_type(TOKEN_LEFTPAR));
    REQUIRE(expect_identifier("bar"));
    REQUIRE(expect_type(TOKEN_LEFTPAR));
    REQUIRE(expect_identifier("a"));
    REQUIRE(expect_type(TOKEN_RIGHTPAR));
    REQUIRE(expect_type(TOKEN_COMMA));
    REQUIRE(expect_identifier("b"));
    REQUIRE(expect_type(TOKEN_RIGHTPAR));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_identifier("c"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_EOF));

    REQUIRE(set_input("a\n    b\n        c"));
    REQUIRE(expect_identifier("a"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_INDENT));
    REQUIRE(expect_identifier("b"));
    REQUIRE(expect_type(TOKEN_EOL));
    REQUIRE(expect_type(TOKEN_INDENT));
    REQUIRE(expect_identifier("c"));
    REQUIRE(expect_type(TOKEN_DEDENT));
    REQUIRE(expect_type(TOKEN_DEDENT));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_crlf(void)
{
    REQUIRE(set_input("a\r\n    b\r\nc\r\n"));
    REQUIRE(expect_type(TOKEN_IDENTIFIER));
    REQUIRE(expect_position(TOKEN_EOL, 1, 2));
    REQUIRE(expect_position(TOKEN_INDENT, 2, 5));
    REQUIRE(expect_identifier("b"));
    REQUIRE(expect_position(TOKEN_EOL, 2, 6));
    REQUIRE(expect_position(TOKEN_DEDENT, 3, 1));
    REQUIRE(expect_identifier("c"));
    REQUIRE(expect_position(TOKEN_EOL, 3, 2));
    REQUIRE(expect_type(TOKEN_EOF));
    return 1;
}

static int test_invalid_inputs(void)
{
    REQUIRE(expect_lexical_error("leading zero", "01\n"));
    REQUIRE(expect_lexical_error("double underscore", "1__2\n"));
    REQUIRE(expect_lexical_error("trailing underscore", "1_\n"));
    REQUIRE(expect_lexical_error("incomplete exponent", "1e+\n"));
    REQUIRE(expect_lexical_error("trailing decimal point", "1.\n"));
    REQUIRE(expect_lexical_error("invalid escape", "\"bad\\q\"\n"));
    REQUIRE(expect_lexical_error("unterminated string", "\"unfinished"));
    REQUIRE(expect_lexical_error("unterminated multiline string", "\"\"\"unfinished\n"));
    REQUIRE(expect_lexical_error("single ampersand", "&\n"));
    REQUIRE(expect_lexical_error("single pipe", "|\n"));
    REQUIRE(expect_lexical_error("tab before code", "a\n\tb\n"));
    REQUIRE(expect_lexical_error("invalid dedent", "a\n    b\n  c\n"));
    REQUIRE(expect_lexical_error("unterminated block comment", "a /* never ends\n"));
    REQUIRE(expect_lexical_error("real CR in single-line string", "\"hello\rworld\"\n"));
    REQUIRE(expect_lexical_error("token after multiline comment", "a /* outer\n inner */ b\n"));
    return 1;
}

typedef int (*TestFunction)(void);

static void run_test(const char *name, TestFunction function,
                     int *passed, int *failed)
{
    int previous_failures = failed_assertions;
    int ok = function();

    if (ok && failed_assertions == previous_failures) {
        printf("PASS: %s\n", name);
        (*passed)++;
    } else {
        printf("FAIL: %s\n", name);
        (*failed)++;
    }
}

int main(void)
{
    int passed = 0;
    int failed = 0;

    run_test("keywords and identifiers", test_keywords_identifiers, &passed, &failed);
    run_test("numeric literals and separators", test_numbers, &passed, &failed);
    run_test("strings and CRLF normalization in multiline strings",
             test_strings_and_crlf_multiline, &passed, &failed);
    run_test("operators and Unit literal", test_operators_and_unit, &passed, &failed);
    run_test("indentation and INDENT/DEDENT source positions",
             test_indentation_positions, &passed, &failed);
    run_test("nested and standalone comments", test_nested_comments_and_comment_lines,
             &passed, &failed);
    run_test("nested parentheses and EOF dedents", test_parentheses_and_eof_dedents,
             &passed, &failed);
    run_test("Windows CRLF input", test_crlf, &passed, &failed);
    run_test("invalid lexical inputs", test_invalid_inputs, &passed, &failed);

    printf("\nAssertions: %d passed, %d failed\n",
           assertions, failed_assertions);
    printf("Test groups: %d passed, %d failed\n", passed, failed);

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
