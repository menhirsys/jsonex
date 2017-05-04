#include <stdio.h>
#include <string.h>

#include "jsonex.h"

int bloop_value = 0;
int blah_snarf_value = 0;
char blah_wharrgbl_value[MAX_STRING_SIZE];
int poop_value = 0;

const char *feed(char *s, size_t len) {
    context_t context;
    jsonex_rule_t rules[] = {
        { .type = JSONEX_INTEGER, .p = &bloop_value, .path = (char *[]){ "blooq", NULL } },
        { .type = JSONEX_INTEGER, .p = &blah_snarf_value, .path = (char *[]){ "blah", "snarf", NULL } },
        { .type = JSONEX_STRING, .p = blah_wharrgbl_value, .path = (char *[]){ "blah", "wharrgbl", NULL } },
        { .type = JSONEX_INTEGER, .p = &poop_value, .path = (char *[]){ "poop", NULL } },
        { .type = JSONEX_NONE }
    };
    jsonex_init(&context, rules);

    for (int i = 0; i < len; i++) {
        if (!jsonex_call(&context, s[i])) {
            return "fail";
        }
    }

    const char *ret = jsonex_finish(&context);

    printf("bloop_value is: %i\n", bloop_value);
    printf("blah_snarf_value is: %i\n", blah_snarf_value);
    printf("blah_wharrgbl_value is: %s\n", blah_wharrgbl_value);
    printf("poop_value is: %i\n", poop_value);

    return ret;
}

int main(void) {
    char *json = " { \"blooq\":42, \"blah\": {\"snarf\": 1234, \"wharrgbl\":  \"mem dog\"} , \"poop\" : 3 } ";//"[1,2,{\"1\":{\"2\":3}}]";//"{\"a\":123.456,\"xyz\":\"qwerty\",\"1\":{\"2\":3}}"; //"494.123"; //"\"ass\"";
    puts(feed(json, strlen(json)));
}
