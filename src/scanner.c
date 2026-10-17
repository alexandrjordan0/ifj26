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

/*------------------------*/
/* STATES OF AUTOMATA     */
/*------------------------*/

typedef enum { place_holder1, place_holder2, place_holder3, place_holder4 } state_t;

/*------------------------*/
/* KEYWORD TABLE            */
/*------------------------*/

static const struct {
    const char *name;
    Token_type type;
} keywords[] = {
    {"place_holder"},
};
