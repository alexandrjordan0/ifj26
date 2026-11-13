// File: test_hashtable.c

//Vytvorené pomocou Claude AI
//Prompt: Vytvor mi testovací program na zadaný hlavičkový súbor hashtable.h.


// Preklad:
//   gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined \
//       test_hashtable.c hashtable.c -o test_hashtable
// Spustenie:
//   ./test_hashtable

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

static int tests_run = 0;
static int tests_failed = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        tests_run++;                                                                   \
        if (!(cond)) {                                                                 \
            tests_failed++;                                                            \
            fprintf(stderr, "  [FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                              \
    } while (0)

#define RUN(test)                                                                      \
    do {                                                                               \
        int before = tests_failed;                                                     \
        printf("Running %-40s", #test);                                                \
        test();                                                                        \
        printf("%s\n", tests_failed == before ? "OK" : "FAILED");                      \
    } while (0)

/* ---------- pomocné funkcie ---------- */

static int free_counter = 0;

static void counting_free(void *p) {
    free_counter++;
    free(p);
}

static int *make_int(int v) {
    int *p = malloc(sizeof *p);
    if (p) *p = v;
    return p;
}

/* ---------- testy ---------- */

static void test_init_zero_capacity(void) {
    CHECK(hashtable_init(0) == NULL);
}

static void test_init_and_delete_empty(void) {
    Htable *ht = hashtable_init(16);
    CHECK(ht != NULL);
    free_counter = 0;
    hashtable_delete(ht, counting_free);
    CHECK(free_counter == 0);
}

static void test_init_capacity_one(void) {
    Htable *ht = hashtable_init(1);
    CHECK(ht != NULL);
    hashtable_delete(ht, counting_free);
}

static void test_search_in_empty(void) {
    Htable *ht = hashtable_init(8);
    CHECK(ht != NULL);
    CHECK(hashtable_search(ht, "nothing") == NULL);
    CHECK(hashtable_search(ht, "") == NULL);
    hashtable_delete(ht, counting_free);
}

static void test_add_and_search(void) {
    Htable *ht = hashtable_init(8);
    CHECK(ht != NULL);

    int *a = make_int(1);
    int *b = make_int(2);

    CHECK(hashtable_add(ht, a, "alpha") == HT_OK);
    CHECK(hashtable_add(ht, b, "beta") == HT_OK);

    CHECK(hashtable_search(ht, "alpha") == a);
    CHECK(hashtable_search(ht, "beta") == b);
    CHECK(*(int *)hashtable_search(ht, "alpha") == 1);
    CHECK(*(int *)hashtable_search(ht, "beta") == 2);
    CHECK(hashtable_search(ht, "gamma") == NULL);

    free_counter = 0;
    hashtable_delete(ht, counting_free);
    CHECK(free_counter == 2);
}

static void test_duplicate_key(void) {
    Htable *ht = hashtable_init(8);
    int *a = make_int(10);
    int *b = make_int(20);

    CHECK(hashtable_add(ht, a, "key") == HT_OK);
    CHECK(hashtable_add(ht, b, "key") == HT_EXISTS);

    /* pôvodná hodnota musí zostať nezmenená */
    CHECK(hashtable_search(ht, "key") == a);
    CHECK(*(int *)hashtable_search(ht, "key") == 10);

    /* b tabuľka nevlastní, lebo add zlyhal – uvoľníme ho sami */
    free(b);

    free_counter = 0;
    hashtable_delete(ht, counting_free);
    CHECK(free_counter == 1);
}

static void test_key_is_copied(void) {
    Htable *ht = hashtable_init(8);
    int *v = make_int(42);

    char buf[32];
    strcpy(buf, "original");
    CHECK(hashtable_add(ht, v, buf) == HT_OK);

    /* zmeníme pôvodný buffer – tabuľka musí mať vlastnú kópiu kľúča */
    strcpy(buf, "modified");
    CHECK(hashtable_search(ht, "original") == v);
    CHECK(hashtable_search(ht, "modified") == NULL);
    CHECK(hashtable_search(ht, buf) == NULL);

    hashtable_delete(ht, counting_free);
}

static void test_empty_string_key(void) {
    Htable *ht = hashtable_init(4);
    int *v = make_int(7);

    CHECK(hashtable_add(ht, v, "") == HT_OK);
    CHECK(hashtable_search(ht, "") == v);
    CHECK(hashtable_add(ht, v, "") == HT_EXISTS);

    hashtable_delete(ht, counting_free);
}

static void test_case_sensitive_and_prefix_keys(void) {
    Htable *ht = hashtable_init(8);
    int *a = make_int(1);
    int *b = make_int(2);
    int *c = make_int(3);

    CHECK(hashtable_add(ht, a, "abc") == HT_OK);
    CHECK(hashtable_add(ht, b, "ABC") == HT_OK);
    CHECK(hashtable_add(ht, c, "ab") == HT_OK);

    CHECK(hashtable_search(ht, "abc") == a);
    CHECK(hashtable_search(ht, "ABC") == b);
    CHECK(hashtable_search(ht, "ab") == c);
    CHECK(hashtable_search(ht, "abcd") == NULL);
    CHECK(hashtable_search(ht, "a") == NULL);

    hashtable_delete(ht, counting_free);
}

static void test_collisions_capacity_one(void) {
    /* kapacita 1 => všetky položky kolidujú, kontrola reťazenia / sondovania */
    Htable *ht = hashtable_init(1);
    CHECK(ht != NULL);

    enum { N = 50 };
    int *vals[N];
    char key[32];

    for (int i = 0; i < N; i++) {
        vals[i] = make_int(i);
        snprintf(key, sizeof key, "key_%d", i);
        CHECK(hashtable_add(ht, vals[i], key) == HT_OK);
    }
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "key_%d", i);
        CHECK(hashtable_search(ht, key) == vals[i]);
    }
    CHECK(hashtable_search(ht, "key_50") == NULL);

    free_counter = 0;
    hashtable_delete(ht, counting_free);
    CHECK(free_counter == N);
}

static void test_many_items(void) {
    enum { N = 5000 };
    Htable *ht = hashtable_init(64);
    CHECK(ht != NULL);

    int **vals = malloc(N * sizeof *vals);
    CHECK(vals != NULL);
    if (!vals) {
        hashtable_delete(ht, counting_free);
        return;
    }
    char key[32];

    for (int i = 0; i < N; i++) {
        vals[i] = make_int(i * 3);
        snprintf(key, sizeof key, "item-%d", i);
        if (hashtable_add(ht, vals[i], key) != HT_OK) {
            CHECK(0 && "add failed");
        }
    }
    int ok = 1;
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "item-%d", i);
        int *got = hashtable_search(ht, key);
        if (got != vals[i] || *got != i * 3) ok = 0;
    }
    CHECK(ok);

    /* opätovné vloženie všetkých kľúčov musí vrátiť HT_EXISTS */
    int all_exist = 1;
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "item-%d", i);
        if (hashtable_add(ht, vals[i], key) != HT_EXISTS) all_exist = 0;
    }
    CHECK(all_exist);

    free(vals);
    free_counter = 0;
    hashtable_delete(ht, counting_free);
    CHECK(free_counter == N);
}

static void test_long_key(void) {
    Htable *ht = hashtable_init(8);
    enum { LEN = 10000 };
    char *key = malloc(LEN + 1);
    CHECK(key != NULL);
    if (!key) {
        hashtable_delete(ht, counting_free);
        return;
    }
    memset(key, 'x', LEN);
    key[LEN] = '\0';

    int *v = make_int(99);
    CHECK(hashtable_add(ht, v, key) == HT_OK);
    CHECK(hashtable_search(ht, key) == v);

    key[LEN - 1] = 'y'; /* líši sa len posledným znakom */
    CHECK(hashtable_search(ht, key) == NULL);

    free(key);
    hashtable_delete(ht, counting_free);
}

static void noop_free(void *p) {
    (void)p;
    free_counter++;
}

static void test_static_values_with_noop(void) {
    static int a = 1, b = 2, c = 3;
    Htable *ht = hashtable_init(4);

    CHECK(hashtable_add(ht, &a, "a") == HT_OK);
    CHECK(hashtable_add(ht, &b, "b") == HT_OK);
    CHECK(hashtable_add(ht, &c, "c") == HT_OK);

    free_counter = 0;
    hashtable_delete(ht, noop_free);
    CHECK(free_counter == 3);
}

static void test_multiple_tables_independent(void) {
    Htable *h1 = hashtable_init(8);
    Htable *h2 = hashtable_init(8);
    int *a = make_int(1);
    int *b = make_int(2);

    CHECK(hashtable_add(h1, a, "shared") == HT_OK);
    CHECK(hashtable_add(h2, b, "shared") == HT_OK); /* iná tabuľka => OK */
    CHECK(hashtable_search(h1, "shared") == a);
    CHECK(hashtable_search(h2, "shared") == b);
    CHECK(hashtable_search(h1, "other") == NULL);

    hashtable_delete(h1, counting_free);
    /* h2 musí po zmazaní h1 fungovať ďalej */
    CHECK(hashtable_search(h2, "shared") == b);
    hashtable_delete(h2, counting_free);
}

int main(void) {
    RUN(test_init_zero_capacity);
    RUN(test_init_and_delete_empty);
    RUN(test_init_capacity_one);
    RUN(test_search_in_empty);
    RUN(test_add_and_search);
    RUN(test_duplicate_key);
    RUN(test_key_is_copied);
    RUN(test_empty_string_key);
    RUN(test_case_sensitive_and_prefix_keys);
    RUN(test_collisions_capacity_one);
    RUN(test_many_items);
    RUN(test_long_key);
    RUN(test_static_values_with_noop);
    RUN(test_multiple_tables_independent);

    printf("\n%d checks, %d failed\n", tests_run, tests_failed);
    return tests_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}