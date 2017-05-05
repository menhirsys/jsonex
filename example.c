#include <stdio.h>

#include "jsonex.h"

int main(void) {
    // This is where jsonex will store the extracted values.
    int a_b_integer;
    char a_c_string[JSONEX_MAX_STRING_SIZE];

    // These are the extraction rules.
    jsonex_rule_t rules[] = {
        // At .a.b, we are looking for a Number (of which we are only
        // interested in the integer part).
        {
            .type = JSONEX_INTEGER,
            .p = &a_b_integer,
            .found = NULL,
            .path = (char *[]){ "a", "b", NULL }
        },

        // At .a.c, we are looking for a String.
        {
            .type = JSONEX_STRING,
            .p = a_c_string,
            .found = NULL,
            .path = (char *[]){ "a", "c", NULL }
        },

        // This is the sentinel telling jsonex that the list of rules is
        // ending. Do not omit!
        { .type = JSONEX_NONE }
    };

    // A jsonex_context_t and jsonex_rule_t[] can be reused as many times as
    // needed, but you must call jsonex_init() on them each time.
    jsonex_context_t context;
    jsonex_init(&context, rules);

    char input[] = "{\"a\":{\"b\":42,\"c\":\"hello there\"}}";
    for (int i = 0; i < sizeof(input) - 1; i++) {
        if (!jsonex_call(&context, input[i])) {
            puts("jsonex_call failed! bad parse!");
            return 1;
        }
    }

    const char *ret;
    if ((ret = jsonex_finish(&context)) != NULL) {
        printf("jsonex_finish: %s\n", ret);
        return 1;
    }

    printf("for input: %s\n", input);
    printf(".a.b = %i\n", a_b_integer);
    printf(".a.c = %s\n", a_c_string);

    return 0;
}
