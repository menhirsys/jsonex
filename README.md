jsonex - an allocations-free, streaming JSON parser/extractor
-

`jsonex` is a JSON parser with useful properties for embedded or
resource-constrained environments:
* it does not use `malloc` or the heap,
* it uses only about 5 frames of stack depth,
* it can be fed one character at a time ("streaming"), rather than requiring
the entire JSON input at once.

`jsonex` parses JSON, but unlike other JSON parsers, it does not return a parse
tree.

Instead, you tell `jsonex` what information you want to extract from the JSON
structure, and then it finds that information for you.

Motivation
-

There are tens of JSON parser implementations, but to my knowledge they all
either construct the parse tree on the heap, or in the case of `jsmn` are not
really parsers.

Also, in spite of JSON being known to be LL(1), I couldn't find a parser that
didn't require the entire JSON input at once.

When you have parsed JSON, what do you do with it? In many cases, you are
just looking for specific information inside of it.

`jsonex` is a JSON parser, that specializes in value extraction, while
requiring as few resources as possible to do it.

Usage
-

(The following is an explanation of `example.c`.)

First, here are where the extracted values will be stored.

```
// This is where jsonex will store the extracted values.
int a_b_integer;
char a_c_string[JSONEX_MAX_STRING_SIZE];
```

Next up, we have the extraction rules. These describe what data we want to
extract from JSON, and where we want to store the extracted data (those
variables we declared earlier).

Extraction rules are path-based, so for example if I want to extract the value
`42` from `{"a":{"b":42}}`, I would use the path `["a", "b"]`.

```
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
```

Now we call `json_init` with a `jsonex_context_t`, and the rules we just
defined.

```
// A jsonex_context_t and jsonex_rule_t[] can be reused as many times as
// needed, but you must call jsonex_init() on them each time.
jsonex_context_t context;
jsonex_init(&context, rules);
```

Then, call `jsonex_call` for every character in our JSON input.

For illustrative purposes, we are feeding in characters from a string literal -
but of course they could come from anywhere - a socket, a file, stdin, etc.
`jsonex` doesn't care what you do between calls to `jsonex_call`.

```
char input[] = "{\"a\":{\"b\":42,\"c\":\"hello there\"}}";
for (int i = 0; i < sizeof(input) - 1; i++) {
    if (!jsonex_call(&context, input[i])) {
        puts("jsonex_call failed! bad parse!");
        return 1;
    }
}
```

When there's no more JSON, call `jsonex_finish`.

```
const char *ret;
if ((ret = jsonex_finish(&context)) != NULL) {
    printf("jsonex_finish: %s\n", ret);
    return 1;
}
```

If `jsonex_finish` was successful, then you have all the values you wanted!

```
printf("for input: %s\n", input);
printf(".a.b = %i\n", a_b_integer);
printf(".a.c = %s\n", a_c_string);
```
