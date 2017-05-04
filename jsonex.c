#if DEBUG
    #include <stdio.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "jsonex.h"

void print_context(const char *, context_t *);

int reap(context_t *context, frame_t *frame_keep_type_and_value) {
    print_context("reap    ", context);
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
        if (frame->is_complete && frame_keep_type_and_value != NULL) {
            frame_keep_type_and_value->type = frame->type;
            frame_keep_type_and_value->u = frame->u;
        }
        return frame->is_complete;
    }
}

void close(context_t *context) {
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
    print_context("close   ", context);
}

void fail(context_t *context) {
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
    print_context("fail    ", context);
}

void replace(context_t *context, parse_fn_t fn) {
    context->frames[context->frames_len - 1].fn = fn;
    print_context("replace ", context);
}

void call(context_t *context, parse_fn_t fn) {
    if (context->frames_len == CONTEXT_FRAME_COUNT) {
        context->error = "context full in call()";
    } else {
        frame_t *frame = &(context->frames[context->frames_len]);
        if (frame->status == FREE) {
            frame->status = IN_USE;
            frame->fn = fn;
            frame->type = JSONEX_NONE;
            context->frames_len++;
        } else {
            context->error = "frame isn't free";
        }
    }
    print_context("call    ", context);
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
    frame->type = JSONEX_NONE;

    replace(context, _literal_null);
    return 0;
}

int value_maybe_null(context_t *context, frame_t *frame, char c) {
    if (reap(context, frame)) {
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
    frame->type = JSONEX_BOOL;

    replace(context, _literal_false);
    return 0;
}

int value_maybe_false(context_t *context, frame_t *frame, char c) {
    if (reap(context, frame)) {
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
    frame->type = JSONEX_BOOL;

    replace(context, _literal_true);
    return 0;
}

int value_maybe_true(context_t *context, frame_t *frame, char c) {
    if (reap(context, frame)) {
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
    if (is_ws(c)) {
        return 1;
    }

    if (reap(context, NULL)) {
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
    if (c == '[') {
        replace(context, array_maybe_empty);
        return 1;
    }

    fail(context);
    return 0;
}

int value_maybe_array(context_t *context, frame_t *frame, char c) {
    if (reap(context, frame)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_true);
        call(context, _true);
        return 0;
    }
}

int object_key(context_t *, frame_t *, char);

void *match_rule(context_t *context, jsonex_type_t type) {
    for (jsonex_rule_t *p = context->rules; p->type != JSONEX_NONE; p++) {
        if (p->type != type) {
            continue;
        }
        int match = 1;
        for (int i = 0; i < context->paths_len; i++) {
            if (p->path[i] == NULL) {
                match = 0;
                break;
            }
            if (strcmp(p->path[i], context->paths[i])) {
                match = 0;
                break;
            }
        }
        if (match) {
            p->found = 1;
            return p->p;
        }
    }
    return NULL;
}

int object_value(context_t *context, frame_t *frame, char c) {
    if (is_ws(c)) {
        return 1;
    }

    if (reap(context, NULL)) {
        frame_t *reaped_frame = &(context->frames[context->frames_len]);
        if (reaped_frame->type != JSONEX_NONE) {
            void *p;
            if ((p = match_rule(context, reaped_frame->type)) != NULL) {
                switch (reaped_frame->type) {
                case JSONEX_INTEGER:
                    *((int *)p) = reaped_frame->u.number.integer_part;
                    break;
                case JSONEX_STRING:
                    strcpy(p, reaped_frame->u.string);
                    break;
                case JSONEX_BOOL:
                    *((int *)p) = reaped_frame->u.boolean;
                    break;
                case JSONEX_NONE:
                    context->error = "got JSONEX_NONE while reap()ing a value";
                    fail(context);
                    return 0;
                }
            }
        }

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
    if (is_ws(c)) {
        return 1;
    }

    if (reap(context, NULL) && c == ':') {
        frame_t *frame = &(context->frames[context->frames_len]);

        // Add key to path.
        strcpy(context->paths[context->paths_len++], frame->u.string);

        replace(context, object_value);
        call(context, value);
        return 1;
    }

    fail(context);
    return 0;
}

int string(context_t *, frame_t *, char);

int object_key(context_t *context, frame_t *frame, char c) {
    if (is_ws(c)) {
        return 1;
    }

    replace(context, object_colon);
    call(context, string);
    return 0;
}

int object_maybe_empty(context_t *context, frame_t *frame, char c) {
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
    if (c == '{') {
        replace(context, object_maybe_empty);
        return 1;
    }

    fail(context);
    return 0;
}

int value_maybe_object(context_t *context, frame_t *frame, char c) {
    if (reap(context, frame)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_array);
        call(context, array);
        return 0;
    }
}

int number_decimal_digits(context_t *context, frame_t *frame, char c) {
    if (c >= '0' && c <= '9') {
        frame->u.number.decimal_digits++;
        frame->u.number.decimal_part += (c - '0') / pow(10, frame->u.number.decimal_digits);
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
    if (c >= '0' && c <= '9') {
        frame->u.number.integer_part *= 10;
        frame->u.number.integer_part += c - '0';
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
    frame->u.number.negative = 0;
    frame->u.number.integer_part = 0;
    frame->u.number.decimal_digits = 0;
    frame->u.number.decimal_part = 0;
    frame->type = JSONEX_INTEGER;

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
    if (reap(context, frame)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_object);
        call(context, object);
        return 0;
    }
}

int string_contents(context_t *context, frame_t *frame, char c) {
    if (c == '"') {
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
    memset(frame->u.string, '\0', sizeof(frame->u.string));
    frame->type = JSONEX_STRING;

    if (c == '"') {
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
    if (reap(context, frame)) {
        close(context);
        return 0;
    } else {
        replace(context, value_maybe_number);
        call(context, number);
        return 0;
    }
}

int value(context_t *context, frame_t *frame, char c) {
    if (is_ws(c)) {
        return 1;
    }

    replace(context, value_maybe_string);
    call(context, string);
    return 0;
}

void print_context(const char *msg, context_t *context) {
#if DEBUG
    printf("%s ", msg);

    if (context->error != NULL) {
        printf("ERROR! %s\n", context->error);
        exit(1);
    }

    for (int i = 0; i < context->frames_len; i++) {
        frame_t *frame = &(context->frames[i]);

        if (frame->status != IN_USE) {
            puts("error! frame->status != IN_USE!");
            return;
        }

        #define FN(f) if (frame->fn == f) { fn_name = #f; } else
        const char *fn_name = "(unk)";
        FN(_literal_null)
        FN(_null)
        FN(value_maybe_null)
        FN(_literal_false)
        FN(_false)
        FN(value_maybe_false)
        FN(_literal_true)
        FN(_true)
        FN(value_maybe_true)
        FN(value)
        FN(array_item)
        FN(array_maybe_empty)
        FN(array)
        FN(value_maybe_array)
        FN(object_key)
        FN(object_value)
        FN(object_colon)
        FN(string)
        FN(object_key)
        FN(object_maybe_empty)
        FN(object)
        FN(value_maybe_object)
        FN(number_decimal_digits)
        FN(number_got_integer_part)
        FN(number_got_nonzero_integer_part)
        FN(number_got_sign)
        FN(number)
        FN(value_maybe_number)
        FN(string_contents)
        FN(string)
        FN(value_maybe_string)
        FN(value)
        {}

        printf(" , %s", fn_name);
    }
    putchar('\n');
#endif
}

void jsonex_init(context_t *context, jsonex_rule_t *rules) {
    context->frames[0].status = IN_USE;
    context->frames[0].fn = value;
    context->frames[0].type = JSONEX_NONE;
    for (int i = 1; i < CONTEXT_FRAME_COUNT; i++) {
        context->frames[i].status = FREE;
    }
    context->frames_len = 1;
    context->paths_len = 0;
    for (jsonex_rule_t *p = rules; p->type != JSONEX_NONE; p++) {
        p->found = 0;
    }
    context->rules = rules;
    context->error = NULL;
}

int jsonex_call(context_t *context, char c) {
    while (context->frames_len > 0) {
        // A parse_fn_t should return truthy if the character was consumed,
        // falsy otherwise.
        char s[9];
#if DEBUG
        sprintf(s, "feed %c  ", c);
#endif
        print_context(s, context);
        frame_t *frame = &(context->frames[context->frames_len - 1]);
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

const char *jsonex_finish(context_t *context) {
    // All parse functions should complete() or abort() when given '\0', so
    // each time we call_context(.., '\0') there should be one less frame.
    while (context->frames_len > 0) {
        size_t old_context_len = context->frames_len;
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
        return NULL;
    }
    return "did not parse";
}
