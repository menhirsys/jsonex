#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jsonex.h"

void run(char *fn, jsonex_rule_t *rules) {
    context_t context;
    jsonex_init(&context, rules);

    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        perror("fopen");
        exit(1);
    }

    int c;
    while ((c = fgetc(f)) != EOF) {
        if (!jsonex_call(&context, c)) {
            printf("%s: jsonex_call() failed\n", fn);
            exit(1);
        }
    }

    const char *ret;
    if ((ret = jsonex_finish(&context)) != NULL) {
        printf("jsonex_finish() during %s: %s\n", fn, ret);
        exit(1);
    }
}

#define CHECK_INTEGER(a, b) \
    if (a != b) { \
        printf(#a " (%i) != %i, while testing %s\n", a, b, fn); \
        exit(1); \
    }

#define CHECK_STRING(a, b) \
    if (strcmp(a, b)) { \
        printf(#a " (%s) != %s, while testing %s\n", a, b, fn); \
        exit(1); \
    }

int main(void) {
    {
        int bloop = 0;
        int blah_snarf = 0;
        char blah_wharrgbl[MAX_STRING_SIZE];
        int pooh = 0;

        jsonex_rule_t rules[] = {
            { .type = JSONEX_INTEGER, .p = &bloop, .path = (char *[]){ "bloop", NULL } },
            { .type = JSONEX_INTEGER, .p = &blah_snarf, .path = (char *[]){ "blah", "snarf", NULL } },
            { .type = JSONEX_STRING, .p = blah_wharrgbl, .path = (char *[]){ "blah", "wharrgbl", NULL } },
            { .type = JSONEX_INTEGER, .p = &pooh, .path = (char *[]){ "pooh", NULL } },
            { .type = JSONEX_NONE }
        };
        char *fn = "tests/1.json";
        run(fn, rules);

        CHECK_INTEGER(bloop, 42);
        CHECK_INTEGER(blah_snarf, 1234);
        CHECK_STRING(blah_wharrgbl, "mem dog");
        CHECK_INTEGER(pooh, 4);
    }

    puts("success!");
}
