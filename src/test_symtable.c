// File: test_symtable.c
// Generovaané pomocou Claude AI
// Prompt: Vytvor mi testovací program na zadaný hlavičkový súbor symtable.h.

// Preklad:
//   gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined test_symtable.c symtable.c
//   hashtable.c -o test_symtable
// (symtable.c môže závisieť aj od ďalších súborov, napr. hashtable.c)
// Spustenie:
//   ./test_symtable
//
// Poznámka: testy s premennými vždy najprv zavolajú symtable_scope_push()
// (pomocná funkcia new_table()), takže nezáleží na tom, či symtable_init()
// sám vytvára globálny scope.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "symtable.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        tests_run++;                                                                               \
        if (!(cond)) {                                                                             \
            tests_failed++;                                                                        \
            fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond);                    \
        }                                                                                          \
    } while (0)

#define RUN(test)                                                                                  \
    do {                                                                                           \
        int before = tests_failed;                                                                 \
        printf("Running %-42s", #test);                                                            \
        test();                                                                                    \
        printf("%s\n", tests_failed == before ? "OK" : "FAILED");                                  \
    } while (0)

/* init + jeden scope, aby sa dali pridávať premenné */
static SymTable *new_table(void) {
    SymTable *st = symtable_init();
    if (st && !symtable_scope_push(st)) {
        symtable_free(st);
        return NULL;
    }
    return st;
}

static int depth_of(SymTable *st, const char *name) {
    VarInfo *v = symtable_find_var(st, name);
    return v ? v->scope_depth : -1;
}

/* ======================= Inicializácia ======================= */

static void test_init_free(void) {
    SymTable *st = symtable_init();
    CHECK(st != NULL);
    symtable_free(st);
}

static void test_empty_lookups(void) {
    SymTable *st = symtable_init();
    CHECK(st != NULL);
    CHECK(symtable_find_var(st, "x") == NULL);
    CHECK(symtable_find_var_current_scope(st, "x") == NULL);
    CHECK(symtable_find_func(st, "f") == NULL);
    symtable_free(st);
}

/* ======================= Premenné ======================= */

static void test_add_and_find_var(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_var(st, "a", TYPE_INT, true) == ERR_OK);
    CHECK(symtable_add_var(st, "b", TYPE_STRING, false) == ERR_OK);
    CHECK(symtable_add_var(st, "c", TYPE_DOUBLE, true) == ERR_OK);
    CHECK(symtable_add_var(st, "d", TYPE_BOOL, false) == ERR_OK);

    VarInfo *a = symtable_find_var(st, "a");
    VarInfo *b = symtable_find_var(st, "b");
    VarInfo *c = symtable_find_var(st, "c");
    VarInfo *d = symtable_find_var(st, "d");

    CHECK(a != NULL && a->type == TYPE_INT && a->is_mutable == true);
    CHECK(b != NULL && b->type == TYPE_STRING && b->is_mutable == false);
    CHECK(c != NULL && c->type == TYPE_DOUBLE && c->is_mutable == true);
    CHECK(d != NULL && d->type == TYPE_BOOL && d->is_mutable == false);

    CHECK(symtable_find_var(st, "nope") == NULL);
    CHECK(symtable_find_var(st, "A") == NULL); /* case-sensitive */
    CHECK(symtable_find_var(st, "") == NULL);

    symtable_free(st);
}

static void test_var_redefinition_same_scope(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_var(st, "x", TYPE_INT, true) == ERR_OK);
    CHECK(symtable_add_var(st, "x", TYPE_INT, true) == ERR_SEM_REDEF);
    CHECK(symtable_add_var(st, "x", TYPE_STRING, false) == ERR_SEM_REDEF);

    /* pôvodná deklarácia musí zostať nezmenená */
    VarInfo *x = symtable_find_var(st, "x");
    CHECK(x != NULL && x->type == TYPE_INT && x->is_mutable == true);

    symtable_free(st);
}

static void test_var_name_is_copied(void) {
    SymTable *st = new_table();

    char buf[16];
    strcpy(buf, "original");
    CHECK(symtable_add_var(st, buf, TYPE_INT, true) == ERR_OK);

    strcpy(buf, "changed!");
    CHECK(symtable_find_var(st, "original") != NULL);
    CHECK(symtable_find_var(st, "changed!") == NULL);

    symtable_free(st);
}

static void test_var_info_modifiable(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_var(st, "v", TYPE_UNIT, true) == ERR_OK);
    VarInfo *v = symtable_find_var(st, "v");
    CHECK(v != NULL);
    if (v)
        v->type = TYPE_DOUBLE;

    VarInfo *again = symtable_find_var(st, "v");
    CHECK(again == v);
    CHECK(again != NULL && again->type == TYPE_DOUBLE);

    symtable_free(st);
}

/* ======================= Scopes ======================= */

static void test_scope_push_pop_depth(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_var(st, "g", TYPE_INT, true) == ERR_OK);
    int d0 = depth_of(st, "g");

    CHECK(symtable_scope_push(st) == true);
    CHECK(symtable_add_var(st, "l1", TYPE_INT, true) == ERR_OK);
    int d1 = depth_of(st, "l1");

    CHECK(symtable_scope_push(st) == true);
    CHECK(symtable_add_var(st, "l2", TYPE_INT, true) == ERR_OK);
    int d2 = depth_of(st, "l2");

    CHECK(d0 >= 0 && d1 >= 0 && d2 >= 0);
    CHECK(d1 == d0 + 1);
    CHECK(d2 == d1 + 1);

    symtable_scope_pop(st);
    symtable_scope_pop(st);

    /* po návrate je nový scope zase na úrovni d1 */
    CHECK(symtable_scope_push(st) == true);
    CHECK(symtable_add_var(st, "l1b", TYPE_INT, true) == ERR_OK);
    CHECK(depth_of(st, "l1b") == d1);
    symtable_scope_pop(st);

    symtable_free(st);
}

static void test_outer_visible_from_inner(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_var(st, "outer", TYPE_INT, true) == ERR_OK);
    CHECK(symtable_scope_push(st) == true);

    CHECK(symtable_find_var(st, "outer") != NULL);               /* vidno */
    CHECK(symtable_find_var_current_scope(st, "outer") == NULL); /* ale nie v aktuálnom */

    CHECK(symtable_add_var(st, "inner", TYPE_STRING, false) == ERR_OK);
    CHECK(symtable_find_var_current_scope(st, "inner") != NULL);

    symtable_scope_pop(st);
    CHECK(symtable_find_var(st, "outer") != NULL);
    CHECK(symtable_find_var_current_scope(st, "outer") != NULL);

    symtable_free(st);
}

static void test_pop_removes_vars(void) {
    SymTable *st = new_table();

    CHECK(symtable_scope_push(st) == true);
    CHECK(symtable_add_var(st, "tmp", TYPE_INT, true) == ERR_OK);
    CHECK(symtable_find_var(st, "tmp") != NULL);
    symtable_scope_pop(st);

    CHECK(symtable_find_var(st, "tmp") == NULL);
    CHECK(symtable_find_var_current_scope(st, "tmp") == NULL);

    /* rovnaké meno sa dá znova použiť */
    CHECK(symtable_scope_push(st) == true);
    CHECK(symtable_add_var(st, "tmp", TYPE_STRING, false) == ERR_OK);
    VarInfo *t = symtable_find_var(st, "tmp");
    CHECK(t != NULL && t->type == TYPE_STRING && t->is_mutable == false);
    symtable_scope_pop(st);

    symtable_free(st);
}

static void test_shadowing(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_var(st, "x", TYPE_INT, true) == ERR_OK);
    VarInfo *outer = symtable_find_var(st, "x");

    CHECK(symtable_scope_push(st) == true);
    /* rovnaké meno vo vnútornom scope je zatienenie, nie redefinícia */
    CHECK(symtable_add_var(st, "x", TYPE_STRING, false) == ERR_OK);

    VarInfo *inner = symtable_find_var(st, "x");
    CHECK(inner != NULL && inner != outer);
    CHECK(inner != NULL && inner->type == TYPE_STRING && inner->is_mutable == false);
    CHECK(symtable_find_var_current_scope(st, "x") == inner);

    /* redefinícia vo vnútornom scope už je chyba */
    CHECK(symtable_add_var(st, "x", TYPE_INT, true) == ERR_SEM_REDEF);

    symtable_scope_pop(st);

    VarInfo *back = symtable_find_var(st, "x");
    CHECK(back != NULL && back->type == TYPE_INT && back->is_mutable == true);

    symtable_free(st);
}

static void test_deep_nesting(void) {
    SymTable *st = new_table();
    enum { DEPTH = 100 };
    char name[32];

    for (int i = 0; i < DEPTH; i++) {
        CHECK(symtable_scope_push(st) == true);
        snprintf(name, sizeof name, "v%d", i);
        CHECK(symtable_add_var(st, name, TYPE_INT, true) == ERR_OK);
    }

    /* zo najhlbšieho scope vidno všetky premenné, v aktuálnom len poslednú */
    int visible = 1;
    for (int i = 0; i < DEPTH; i++) {
        snprintf(name, sizeof name, "v%d", i);
        if (symtable_find_var(st, name) == NULL)
            visible = 0;
    }
    CHECK(visible);
    CHECK(symtable_find_var_current_scope(st, "v99") != NULL);
    CHECK(symtable_find_var_current_scope(st, "v98") == NULL);

    for (int i = DEPTH - 1; i >= 0; i--) {
        snprintf(name, sizeof name, "v%d", i);
        CHECK(symtable_find_var(st, name) != NULL);
        symtable_scope_pop(st);
        CHECK(symtable_find_var(st, name) == NULL);
    }

    symtable_free(st);
}

static void test_many_vars_one_scope(void) {
    SymTable *st = new_table();
    enum { N = 2000 };
    char name[32];

    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof name, "var_%d", i);
        if (symtable_add_var(st, name, (i % 2) ? TYPE_INT : TYPE_DOUBLE, i % 3 == 0) != ERR_OK)
            CHECK(0 && "add_var failed");
    }
    int ok = 1;
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof name, "var_%d", i);
        VarInfo *v = symtable_find_var(st, name);
        if (!v || v->type != ((i % 2) ? TYPE_INT : TYPE_DOUBLE) || v->is_mutable != (i % 3 == 0))
            ok = 0;
    }
    CHECK(ok);

    symtable_free(st);
}

/* ======================= Funkcie ======================= */

static void test_add_and_find_func(void) {
    SymTable *st = new_table();

    Type types[] = {TYPE_INT, TYPE_STRING, TYPE_DOUBLE};
    const char *names[] = {"a", "b", "c"};

    CHECK(symtable_add_func(st, "foo", 3, types, names, TYPE_BOOL, true) == ERR_OK);

    FuncInfo *f = symtable_find_func(st, "foo");
    CHECK(f != NULL);
    if (f) {
        CHECK(f->name != NULL && strcmp(f->name, "foo") == 0);
        CHECK(f->param_count == 3);
        CHECK(f->param_types != NULL);
        CHECK(f->param_names != NULL);
        if (f->param_types && f->param_names) {
            CHECK(f->param_types[0] == TYPE_INT);
            CHECK(f->param_types[1] == TYPE_STRING);
            CHECK(f->param_types[2] == TYPE_DOUBLE);
            CHECK(strcmp(f->param_names[0], "a") == 0);
            CHECK(strcmp(f->param_names[1], "b") == 0);
            CHECK(strcmp(f->param_names[2], "c") == 0);
        }
        CHECK(f->return_type == TYPE_BOOL);
        CHECK(f->is_defined == true);
    }

    CHECK(symtable_find_func(st, "bar") == NULL);
    CHECK(symtable_find_func(st, "Foo") == NULL);

    symtable_free(st);
}

static void test_func_no_params(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_func(st, "noargs", 0, NULL, NULL, TYPE_UNIT, false) == ERR_OK);
    FuncInfo *f = symtable_find_func(st, "noargs");
    CHECK(f != NULL);
    if (f) {
        CHECK(f->param_count == 0);
        CHECK(f->param_types == NULL); /* podľa hlavičky */
        CHECK(f->return_type == TYPE_UNIT);
        CHECK(f->is_defined == false);
    }

    symtable_free(st);
}

static void test_func_unknown_return_type(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_func(st, "later", 0, NULL, NULL, TYPE_UNKNOWN, false) == ERR_OK);
    FuncInfo *f = symtable_find_func(st, "later");
    CHECK(f != NULL && f->return_type == TYPE_UNKNOWN);

    /* hodnoty vrátené ukazovateľom sa dajú doplniť neskôr */
    if (f) {
        f->return_type = TYPE_INT;
        f->is_defined = true;
    }
    FuncInfo *g = symtable_find_func(st, "later");
    CHECK(g == f);
    CHECK(g != NULL && g->return_type == TYPE_INT && g->is_defined == true);

    symtable_free(st);
}

static void test_func_data_is_copied(void) {
    SymTable *st = new_table();

    char fname[16];
    strcpy(fname, "copied");

    char p0[8], p1[8];
    strcpy(p0, "x");
    strcpy(p1, "y");
    const char *names[] = {p0, p1};
    Type types[] = {TYPE_INT, TYPE_DOUBLE};

    CHECK(symtable_add_func(st, fname, 2, types, names, TYPE_INT, true) == ERR_OK);

    /* zničíme všetky pôvodné dáta */
    strcpy(fname, "XXXXXX");
    strcpy(p0, "q");
    strcpy(p1, "w");
    types[0] = TYPE_STRING;
    types[1] = TYPE_STRING;

    FuncInfo *f = symtable_find_func(st, "copied");
    CHECK(f != NULL);
    CHECK(symtable_find_func(st, "XXXXXX") == NULL);
    if (f && f->param_types && f->param_names) {
        CHECK(strcmp(f->name, "copied") == 0);
        CHECK(f->param_types[0] == TYPE_INT);
        CHECK(f->param_types[1] == TYPE_DOUBLE);
        CHECK(strcmp(f->param_names[0], "x") == 0);
        CHECK(strcmp(f->param_names[1], "y") == 0);
        CHECK(f->param_types != types);
        CHECK(f->param_names[0] != p0);
    } else {
        CHECK(0 && "function data missing");
    }

    symtable_free(st);
}

static void test_func_redefinition(void) {
    SymTable *st = new_table();
    Type t[] = {TYPE_INT};
    const char *n[] = {"p"};

    CHECK(symtable_add_func(st, "dup", 1, t, n, TYPE_INT, true) == ERR_OK);
    CHECK(symtable_add_func(st, "dup", 1, t, n, TYPE_INT, true) == ERR_SEM_REDEF);
    CHECK(symtable_add_func(st, "dup", 0, NULL, NULL, TYPE_STRING, false) == ERR_SEM_REDEF);

    /* pôvodná funkcia zostane nezmenená */
    FuncInfo *f = symtable_find_func(st, "dup");
    CHECK(f != NULL && f->param_count == 1 && f->return_type == TYPE_INT && f->is_defined);

    symtable_free(st);
}

static void test_many_funcs(void) {
    SymTable *st = new_table();
    enum { N = 1000 };
    char name[32];
    Type t[] = {TYPE_INT, TYPE_ANY};
    const char *pn[] = {"a", "b"};

    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof name, "func_%d", i);
        if (symtable_add_func(st, name, 2, t, pn, TYPE_INT, true) != ERR_OK)
            CHECK(0 && "add_func failed");
    }
    int ok = 1;
    for (int i = 0; i < N; i++) {
        snprintf(name, sizeof name, "func_%d", i);
        FuncInfo *f = symtable_find_func(st, name);
        if (!f || strcmp(f->name, name) != 0 || f->param_count != 2)
            ok = 0;
    }
    CHECK(ok);

    symtable_free(st);
}

/* ======================= Funkcie vs. premenné ======================= */

static void test_funcs_and_vars_independent(void) {
    SymTable *st = new_table();

    CHECK(symtable_add_func(st, "same", 0, NULL, NULL, TYPE_INT, true) == ERR_OK);
    CHECK(symtable_add_var(st, "same", TYPE_STRING, true) == ERR_OK);

    CHECK(symtable_find_func(st, "same") != NULL);
    CHECK(symtable_find_var(st, "same") != NULL);
    VarInfo *sv = symtable_find_var(st, "same");
    FuncInfo *sf = symtable_find_func(st, "same");
    CHECK(sv != NULL && sv->type == TYPE_STRING);
    CHECK(sf != NULL && sf->return_type == TYPE_INT);

    /* scope pop sa funkcií netýka */
    CHECK(symtable_scope_push(st) == true);
    symtable_scope_pop(st);
    CHECK(symtable_find_func(st, "same") != NULL);

    symtable_free(st);
}

static void test_func_params_not_in_vars(void) {
    SymTable *st = new_table();
    Type t[] = {TYPE_INT};
    const char *n[] = {"param"};

    CHECK(symtable_add_func(st, "withparam", 1, t, n, TYPE_UNIT, true) == ERR_OK);
    /* samotné pridanie funkcie nesmie vytvoriť premennú s názvom parametra */
    CHECK(symtable_find_var(st, "param") == NULL);

    symtable_free(st);
}

int main(void) {
    RUN(test_init_free);
    RUN(test_empty_lookups);

    RUN(test_add_and_find_var);
    RUN(test_var_redefinition_same_scope);
    RUN(test_var_name_is_copied);
    RUN(test_var_info_modifiable);

    RUN(test_scope_push_pop_depth);
    RUN(test_outer_visible_from_inner);
    RUN(test_pop_removes_vars);
    RUN(test_shadowing);
    RUN(test_deep_nesting);
    RUN(test_many_vars_one_scope);

    RUN(test_add_and_find_func);
    RUN(test_func_no_params);
    RUN(test_func_unknown_return_type);
    RUN(test_func_data_is_copied);
    RUN(test_func_redefinition);
    RUN(test_many_funcs);

    RUN(test_funcs_and_vars_independent);
    RUN(test_func_params_not_in_vars);

    printf("\n%d checks, %d failed\n", tests_run, tests_failed);
    return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}