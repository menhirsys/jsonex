#include <stdio.h>
#include <string.h>

#include <math.h>
#include <stddef.h>

#define CONTEXT_FRAME_COUNT 16
#define MAX_STRING_SIZE 64

struct context;
struct frame;

typedef int (*parse_fn_t)(struct context *, struct frame *, char);

typedef struct frame {
    enum {
        FREE,
        IN_USE,
        ZOMBIE
    } status;
    union {
        struct {
            const char *string;
            size_t len;
            size_t offset;
        } literal;
        struct {
            int negative;
            int integer_part;
            int decimal_digits;
            double decimal_part;
        } number;
        char string[MAX_STRING_SIZE];
    } u;
    parse_fn_t fn;
    int is_complete;
} frame_t;

typedef struct {
    enum {
        JSONEX_INTEGER,
        JSONEX_END
    } type;
    void *p;
    char **path;
    int found;
} jsonex_rule_t;

typedef struct context {
    frame_t frames[CONTEXT_FRAME_COUNT];
    size_t frames_len;
    char paths[CONTEXT_FRAME_COUNT][MAX_STRING_SIZE];
    size_t paths_len;
    jsonex_rule_t *rules;
    const char *error;
} context_t;

int reap(context_t *context) {
    puts(".. reap()");
    if (context->frames_len == CONTEXT_FRAME_COUNT) {
        context->error = "context full in reap()";
        return 0;
    }
    frame_t *frame = &(context->frames[context->frames_len]);
    if (frame->status != ZOMBIE) {
        context->error = "frame isn't a zombie";
        return 0;
    } else {
        frame->status = FREE;
        return frame->is_complete;
    }
}

void close(context_t *context) {
    puts(".. close()");
    if (context->frames_len == 0) {
        context->error = "context empty in close()"; // todo report error somehow. also init to NULL. also don't clobber
    } else {
        frame_t *frame = &(context->frames[context->frames_len - 1]);
        if (frame->status != IN_USE) {
            context->error = "frame isn't in use";
        } else {
            frame->status = ZOMBIE;
            frame->is_complete = 1;
            context->frames_len--;
        }
    }
}

void fail(context_t *context) {
    puts(".. fail()");
    if (context->frames_len == 0) {
        context->error = "context empty in fail()";
    } else {
        frame_t *frame = &(context->frames[context->frames_len - 1]);
        if (frame->status != IN_USE) {
            context->error = "frame isn't in use";
        } else {
            frame->status = ZOMBIE;
            frame->is_complete = 0;
            context->frames_len--;
        }
    }
}

void replace(context_t *context, parse_fn_t fn) {
    puts(".. replace()");
    context->frames[context->frames_len - 1].fn = fn;
}

void call(context_t *context, parse_fn_t fn) {
    puts(".. call()");
    if (context->frames_len == CONTEXT_FRAME_COUNT) {
        context->error = "context full in call()";
    } else {
        frame_t *frame = &(context->frames[context->frames_len]);
        if (frame->status == FREE) {
            frame->status = IN_USE;
            frame->fn = fn;
            context->frames_len++;
        } else {
            context->error = "frame isn't free";
        }
    }
}

int is_ws(char c) {
    switch (c) {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
        return 1;
    }
    return 0;
}

int _literal_null(context_t *context, frame_t *frame, char c) {
    if (c == frame->u.literal.string[frame->u.literal.offset]) {
        frame->u.literal.offset++;
        if (frame->u.literal.offset == frame->u.literal.len) {
            close(context);
        }
        return 1;
    }

    fail(context);
    return 0;
}

int _null(context_t *context, frame_t *frame, char c) {
    frame->u.literal.string = "null";
    frame->u.literal.len = 4;
    frame->u.literal.offset = 0;

    replace(context, _literal_null);
    return 0;
}

int value_maybe_null(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_null");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        fail(context);
        return 0;
    }
}

int _literal_false(context_t *context, frame_t *frame, char c) {
    if (c == frame->u.literal.string[frame->u.literal.offset]) {
        frame->u.literal.offset++;
        if (frame->u.literal.offset == frame->u.literal.len) {
            close(context);
        }
        return 1;
    }

    fail(context);
    return 0;
}

int _false(context_t *context, frame_t *frame, char c) {
    frame->u.literal.string = "false";
    frame->u.literal.len = 5;
    frame->u.literal.offset = 0;

    replace(context, _literal_false);
    return 0;
}

int value_maybe_false(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_false");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_null);
        call(context, _null);
        return 0;
    }
}

int _literal_true(context_t *context, frame_t *frame, char c) {
    if (c == frame->u.literal.string[frame->u.literal.offset]) {
        frame->u.literal.offset++;
        if (frame->u.literal.offset == frame->u.literal.len) {
            close(context);
        }
        return 1;
    }

    fail(context);
    return 0;
}

int _true(context_t *context, frame_t *frame, char c) {
    frame->u.literal.string = "true";
    frame->u.literal.len = 4;
    frame->u.literal.offset = 0;

    replace(context, _literal_true);
    return 0;
}

int value_maybe_true(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_true");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_false);
        call(context, _false);
        return 0;
    }
}

int value(context_t *, frame_t *, char);

int array_item(context_t *context, frame_t *frame, char c) {
    puts("array_item");

    if (is_ws(c)) {
        return 1;
    }

    if (reap(context)) {
        if (c == ',') {
            call(context, value);
            return 1;
        } else if (c == ']') {
            close(context);
            return 1;
        }
    }

    fail(context);
    return 0;
}

int array_maybe_empty(context_t *context, frame_t *frame, char c) {
    puts("array_maybe_empty");

    if (c == ']') {
        // The empty array.
        close(context);
        return 1;
    } else if (c == '\0') {
        fail(context);
        return 0;
    } else {
        replace(context, array_item);
        call(context, value);
        return 0;
    }
}

int array(context_t *context, frame_t *frame, char c) {
    puts("array");

    if (c == '[') {
        replace(context, array_maybe_empty);
        return 1;
    }

    fail(context);
    return 0;
}

int value_maybe_array(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_array");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_true);
        call(context, _true);
        return 0;
    }
}

int object_key(context_t *, frame_t *, char);

int object_value(context_t *context, frame_t *frame, char c) {
    puts("object_value");

    if (is_ws(c)) {
        return 1;
    }

    if (reap(context)) {
        // Remove last path component.
        // todo check 0
        context->paths_len--;

        if (c == ',') {
            replace(context, object_key);
            return 1;
        } else if (c == '}') {
            close(context);
            return 1;
        }
    }

    fail(context);
    return 0;
}

// todo static
int object_colon(context_t *context, frame_t *frame, char c) {
    puts("object_colon");

    if (is_ws(c)) {
        return 1;
    }

    if (reap(context) && c == ':') {
        frame_t *frame = &(context->frames[context->frames_len]);

        // Add key to path.
        strcpy(context->paths[context->paths_len++], frame->u.string);
        printf("path now:");
        for (int i = 0; i < context->paths_len; i++) {
            printf(" %s", context->paths[i]);
        }
        puts("");

        replace(context, object_value);
        call(context, value);
        return 1;
    }

    fail(context);
    return 0;
}

int string(context_t *, frame_t *, char);

int object_key(context_t *context, frame_t *frame, char c) {
    puts("object_key");

    if (is_ws(c)) {
        return 1;
    }

    replace(context, object_colon);
    call(context, string);
    return 0;
}

int object_maybe_empty(context_t *context, frame_t *frame, char c) {
    puts("object_maybe_empty");

    if (is_ws(c)) {
        return 1;
    }

    if (c == '}') {
        // The empty object.
        close(context);
        return 1;
    } else if (c == '\0') {
        fail(context);
        return 0;
    } else {
        replace(context, object_key);
        return 0;
    }
}

int object(context_t *context, frame_t *frame, char c) {
    puts("object");

    if (c == '{') {
        replace(context, object_maybe_empty);
        return 1;
    }

    fail(context);
    return 0;
}

int value_maybe_object(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_object");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_array);
        call(context, array);
        return 0;
    }
}

int number_decimal_digits(context_t *context, frame_t *frame, char c) {
    puts("number_decimal_digits");

    if (c >= '0' && c <= '9') {
        frame->u.number.decimal_digits++;
        frame->u.number.decimal_part += (c - '0') / pow(10, frame->u.number.decimal_digits);
        printf(" .. number so far = %f\n", frame->u.number.integer_part + frame->u.number.decimal_part);
        return 1;
    } else if (frame->u.number.decimal_digits > 0) {
        // As long as we get one digit after ., it's a valid number.
        close(context);
        return 0;
    } else {
        fail(context);
        return 0;
    }
    // todo: handle e+4
}

int number_got_integer_part(context_t *context, frame_t *frame, char c) {
    puts("number_got_integer_part");

    if (c == '.') {
        replace(context, number_decimal_digits);
        return 1;
    } else {
        // Input can end here, and we still have a number.
        close(context);
        return 0;
    }
}

int number_got_nonzero_integer_part(context_t *context, frame_t *frame, char c) {
    puts("number_got_nonzero_integer_part");

    if (c >= '0' && c <= '9') {
        frame->u.number.integer_part *= 10;
        frame->u.number.integer_part += c - '0';
        printf(" .. number so far = %i\n", frame->u.number.integer_part);
        return 1;
    } else if (c == '\0') {
        // We always get a digit first (see number_got_sign), so if we get a \0
        // here that's fine, we still got a number.
        close(context);
        return 0;
    } else {
        replace(context, number_got_integer_part);
        return 0;
    }
}

int number_got_sign(context_t *context, frame_t *frame, char c) {
    puts("number_got_sign");

    if (c == '0') {
        replace(context, number_got_integer_part);
        return 1;
    } else if (c >= '1' && c <= '9') {
        replace(context, number_got_nonzero_integer_part);
        return 0;
    } else {
        // Got the sign but no digit, if we get anything but [0-9] at this
        // point (including \0), it's a fail.
        fail(context);
        return 0;
    }
}

int number(context_t *context, frame_t *frame, char c) {
    puts("number");

    frame->u.number.negative = 0;
    frame->u.number.integer_part = 0;
    frame->u.number.decimal_digits = 0;
    frame->u.number.decimal_part = 0;

    if (c == '\0') {
        fail(context);
        return 0;
    } else {
        replace(context, number_got_sign);
        if (c == '-') {
            frame->u.number.negative = 1;
            return 1;
        } else {
            return 0;
        }
    }
}

int value_maybe_number(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_number");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_object);
        call(context, object);
        return 0;
    }
}

int string_contents(context_t *context, frame_t *frame, char c) {
    puts("string_contents");
    if (c == '"') {
        // todo check length
        printf("string_contents: got string close, string was %s\n", frame->u.string);
        close(context);
        return 1;
    } else if (c == '\0') {
        fail(context);
        return 0;
    } else {
        // Add to buffer and consume.
        // todo if too long error somehow
        frame->u.string[strlen(frame->u.string)] = c;
        return 1;
    }
    // todo handle escaping
}

int string(context_t *context, frame_t *frame, char c) {
    puts("string");

    memset(frame->u.string, '\0', sizeof(frame->u.string));

    if (c == '"') {
        puts("string: got string open");
        replace(context, string_contents);
        return 1;
    } else if (c == '\0') {
        fail(context);
        return 0;
    } else {
        fail(context);
        return 0;
    }
}

int value_maybe_string(context_t *context, frame_t *frame, char c) {
    puts("value_maybe_string");
    if (reap(context)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_number);
        call(context, number);
        return 0;
    }
}

int value(context_t *context, frame_t *frame, char c) {
    puts("value");

    if (is_ws(c)) {
        return 1;
    }

    replace(context, value_maybe_string);
    call(context, string);
    return 0;
}

void jsonex_init(context_t *context, jsonex_rule_t *rules) {
    context->frames[0].status = IN_USE;
    context->frames[0].fn = value;
    for (int i = 1; i < CONTEXT_FRAME_COUNT; i++) {
        context->frames[i].status = FREE;
    }
    context->frames_len = 1;
    context->paths_len = 0;
    context->rules = rules;
    context->error = NULL;
}

int jsonex_call(context_t *context, char c) {
    while (context->frames_len > 0) {
        // A parse_fn_t should return truthy if the character was consumed,
        // falsy otherwise.
        frame_t *frame = &(context->frames[context->frames_len - 1]);
        size_t old_context_len = context->frames_len;
        if (frame->fn(context, frame, c)) {
            return 1;
        }
        // Keep looping until something consumes this character! (Or there's no
        // more context.)
    }

    // Eat up trailing whitespace.
    if (is_ws(c)) {
        return 1;
    }

    return 0;
}

const char *jsonex_success = "success";
const char *jsonex_fail = "fail";

const char *jsonex_finish(context_t *context) {
    // All parse functions should complete() or abort() when given '\0', so
    // each time we call_context(.., '\0') there should be one less frame.
    while (context->frames_len > 0) {
        size_t old_context_len = context->frames_len;
        printf("finishing, c = '\\0'\n");
        if (jsonex_call(context, '\0')) {
            return "internal error: some parse function consumed '\\0'";
        }
        if (context->frames_len >= old_context_len) {
            return "internal error: context->frames_len not decreasing";
        }
    }

    // Since init_context() put something on the context, and there was nothing
    // left to reap() it, we ought to have a zombie left over telling us
    // whether the parse succeeded or failed.
    if (context->frames[0].status != ZOMBIE) {
        return "internal error: first frame isn't a zombie";
    }
    if (context->frames[0].is_complete) {
        return jsonex_success;
    }
    return jsonex_fail;
}

const char *feed(char *s, size_t len) {
    context_t context;
    jsonex_rule_t rules[] = {
        { .type = JSONEX_INTEGER, .p = &value, .path = (char *[]){ "bloop", NULL } },
        { .type = JSONEX_END }
    };
    jsonex_init(&context, rules);

    for (int i = 0; i < len; i++) {
        printf("c = %c\n", s[i]);
        if (!jsonex_call(&context, s[i])) {
            return jsonex_fail;
        }
    }

    return jsonex_finish(&context);
}

int main(void) {
    char *json = " { \"bloop\":42, \"blah\": {\"snarf\": \"thundercat\", \"wharrgbl\":  \"mem dog\"} , \"poop\" : 3 } ";//"[1,2,{\"1\":{\"2\":3}}]";//"{\"a\":123.456,\"xyz\":\"qwerty\",\"1\":{\"2\":3}}"; //"494.123"; //"\"ass\"";
    puts(feed(json, strlen(json)));
}
