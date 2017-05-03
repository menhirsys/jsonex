#include <stdio.h>
#include <string.h>

#include <math.h>
#include <stddef.h>

#define STACK_FRAME_COUNT 16

struct stack;
struct frame;

typedef int (*parse_fn_t)(struct stack *, struct frame *, char);

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
        struct {
            char string[64];
            size_t len;
        } string;
    } u;
    parse_fn_t fn;
    int is_complete;
} frame_t;

typedef struct stack {
    frame_t frames[STACK_FRAME_COUNT];
    size_t len;
    const char *error;
} stack_t;

int reap(stack_t *stack) {
    puts(".. reap()");
    if (stack->len == STACK_FRAME_COUNT) {
        stack->error = "stack full in reap()";
        return 0;
    } else {
        return stack->frames[stack->len].is_complete;
    }
}

void close(stack_t *stack) {
    puts(".. close()");
    if (stack->len == 0) {
        stack->error = "stack empty in close()"; // todo report error somehow. also init to NULL. also don't clobber
    } else {
        frame_t *frame = &(stack->frames[stack->len - 1]);
        frame->status = ZOMBIE;
        frame->is_complete = 1;
        stack->len--;
    }
}

void fail(stack_t *stack) {
    puts(".. fail()");
    if (stack->len == 0) {
        stack->error = "stack empty in fail()";
    } else {
        frame_t *frame = &(stack->frames[stack->len - 1]);
        frame->status = ZOMBIE;
        frame->is_complete = 0;
        stack->len--;
    }
}

void replace(stack_t *stack, parse_fn_t fn) {
    puts(".. replace()");
    stack->frames[stack->len - 1].fn = fn;
}

void call(stack_t *stack, parse_fn_t fn) {
    puts(".. call()");
    if (stack->len == STACK_FRAME_COUNT) {
        stack->error = "stack full in call()";
    } else {
        frame_t *frame = &(stack->frames[stack->len]);
        frame->status = IN_USE;
        frame->fn = fn;
        stack->len++;
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

int _literal_null(stack_t *stack, frame_t *frame, char c) {
    if (c == frame->u.literal.string[frame->u.literal.offset]) {
        frame->u.literal.offset++;
        if (frame->u.literal.offset == frame->u.literal.len) {
            close(stack);
        }
        return 1;
    }

    fail(stack);
    return 0;
}

int _null(stack_t *stack, frame_t *frame, char c) {
    frame->u.literal.string = "null";
    frame->u.literal.len = 4;
    frame->u.literal.offset = 0;

    replace(stack, _literal_null);
    return 0;
}

int value_maybe_null(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_null");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        fail(stack);
        return 0;
    }
}

int _literal_false(stack_t *stack, frame_t *frame, char c) {
    if (c == frame->u.literal.string[frame->u.literal.offset]) {
        frame->u.literal.offset++;
        if (frame->u.literal.offset == frame->u.literal.len) {
            close(stack);
        }
        return 1;
    }

    fail(stack);
    return 0;
}

int _false(stack_t *stack, frame_t *frame, char c) {
    frame->u.literal.string = "false";
    frame->u.literal.len = 5;
    frame->u.literal.offset = 0;

    replace(stack, _literal_false);
    return 0;
}

int value_maybe_false(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_false");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        replace(stack, value_maybe_null);
        call(stack, _null);
        return 0;
    }
}

int _literal_true(stack_t *stack, frame_t *frame, char c) {
    if (c == frame->u.literal.string[frame->u.literal.offset]) {
        frame->u.literal.offset++;
        if (frame->u.literal.offset == frame->u.literal.len) {
            close(stack);
        }
        return 1;
    }

    fail(stack);
    return 0;
}

int _true(stack_t *stack, frame_t *frame, char c) {
    frame->u.literal.string = "true";
    frame->u.literal.len = 4;
    frame->u.literal.offset = 0;

    replace(stack, _literal_true);
    return 0;
}

int value_maybe_true(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_true");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        replace(stack, value_maybe_false);
        call(stack, _false);
        return 0;
    }
}

int value(stack_t *, frame_t *, char);

int array_item(stack_t *stack, frame_t *frame, char c) {
    puts("array_item");

    if (is_ws(c)) {
        return 1;
    }

    if (reap(stack)) {
        if (c == ',') {
            call(stack, value);
            return 1;
        } else if (c == ']') {
            close(stack);
            return 1;
        }
    }

    fail(stack);
    return 0;
}

int array_maybe_empty(stack_t *stack, frame_t *frame, char c) {
    puts("array_maybe_empty");

    if (c == ']') {
        // The empty array.
        close(stack);
        return 1;
    } else if (c == '\0') {
        fail(stack);
        return 0;
    } else {
        replace(stack, array_item);
        call(stack, value);
        return 0;
    }
}

int array(stack_t *stack, frame_t *frame, char c) {
    puts("array");

    if (c == '[') {
        replace(stack, array_maybe_empty);
        return 1;
    }

    fail(stack);
    return 0;
}

int value_maybe_array(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_array");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        replace(stack, value_maybe_true);
        call(stack, _true);
        return 0;
    }
}

int object_key(stack_t *, frame_t *, char);

int object_value(stack_t *stack, frame_t *frame, char c) {
    puts("object_value");

    if (is_ws(c)) {
        return 1;
    }

    if (reap(stack)) {
        if (c == ',') {
            replace(stack, object_key);
            return 1;
        } else if (c == '}') {
            close(stack);
            return 1;
        }
    }

    fail(stack);
    return 0;
}

// todo static
int object_colon(stack_t *stack, frame_t *frame, char c) {
    puts("object_colon");

    if (is_ws(c)) {
        return 1;
    }

    if (reap(stack) && c == ':') {
        replace(stack, object_value);
        call(stack, value);
        return 1;
    }

    fail(stack);
    return 0;
}

int string(stack_t *, frame_t *, char);

int object_key(stack_t *stack, frame_t *frame, char c) {
    puts("object_key");

    if (is_ws(c)) {
        return 1;
    }

    replace(stack, object_colon);
    call(stack, string);
    return 0;
}

int object_maybe_empty(stack_t *stack, frame_t *frame, char c) {
    puts("object_maybe_empty");

    if (is_ws(c)) {
        return 1;
    }

    if (c == '}') {
        // The empty object.
        close(stack);
        return 1;
    } else if (c == '\0') {
        fail(stack);
        return 0;
    } else {
        replace(stack, object_key);
        return 0;
    }
}

int object(stack_t *stack, frame_t *frame, char c) {
    puts("object");

    if (c == '{') {
        replace(stack, object_maybe_empty);
        return 1;
    }

    fail(stack);
    return 0;
}

int value_maybe_object(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_object");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        replace(stack, value_maybe_array);
        call(stack, array);
        return 0;
    }
}

int number_decimal_digits(stack_t *stack, frame_t *frame, char c) {
    puts("number_decimal_digits");

    if (c >= '0' && c <= '9') {
        frame->u.number.decimal_digits++;
        frame->u.number.decimal_part += (c - '0') / pow(10, frame->u.number.decimal_digits);
        printf(" .. number so far = %f\n", frame->u.number.integer_part + frame->u.number.decimal_part);
        return 1;
    } else if (frame->u.number.decimal_digits > 0) {
        // As long as we get one digit after ., it's a valid number.
        close(stack);
        return 0;
    } else {
        fail(stack);
        return 0;
    }
    // todo: handle e+4
}

int number_got_integer_part(stack_t *stack, frame_t *frame, char c) {
    puts("number_got_integer_part");

    if (c == '.') {
        replace(stack, number_decimal_digits);
        return 1;
    } else {
        // Input can end here, and we still have a number.
        close(stack);
        return 0;
    }
}

int number_got_nonzero_integer_part(stack_t *stack, frame_t *frame, char c) {
    puts("number_got_nonzero_integer_part");

    if (c >= '0' && c <= '9') {
        frame->u.number.integer_part *= 10;
        frame->u.number.integer_part += c - '0';
        printf(" .. number so far = %i\n", frame->u.number.integer_part);
        return 1;
    } else if (c == '\0') {
        // We always get a digit first (see number_got_sign), so if we get a \0
        // here that's fine, we still got a number.
        close(stack);
        return 0;
    } else {
        replace(stack, number_got_integer_part);
        return 0;
    }
}

int number_got_sign(stack_t *stack, frame_t *frame, char c) {
    puts("number_got_sign");

    if (c == '0') {
        replace(stack, number_got_integer_part);
        return 1;
    } else if (c >= '1' && c <= '9') {
        replace(stack, number_got_nonzero_integer_part);
        return 0;
    } else {
        // Got the sign but no digit, if we get anything but [0-9] at this
        // point (including \0), it's a fail.
        fail(stack);
        return 0;
    }
}

int number(stack_t *stack, frame_t *frame, char c) {
    puts("number");

    frame->u.number.negative = 0;
    frame->u.number.integer_part = 0;
    frame->u.number.decimal_digits = 0;
    frame->u.number.decimal_part = 0;

    if (c == '\0') {
        fail(stack);
        return 0;
    } else {
        replace(stack, number_got_sign);
        if (c == '-') {
            frame->u.number.negative = 1;
            return 1;
        } else {
            return 0;
        }
    }
}

int value_maybe_number(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_number");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        replace(stack, value_maybe_object);
        call(stack, object);
        return 0;
    }
}

int string_contents(stack_t *stack, frame_t *frame, char c) {
    puts("string_contents");
    if (c == '"') {
        printf("string_contents: got string close, string was %.*s\n", frame->u.string.len, frame->u.string.string);
        close(stack);
        return 1;
    } else if (c == '\0') {
        fail(stack);
        return 0;
    } else {
        // Add to buffer and consume.
        // todo if too long error somehow
        frame->u.string.string[frame->u.string.len++] = c;
        return 1;
    }
    // todo handle escaping
}

int string(stack_t *stack, frame_t *frame, char c) {
    puts("string");

    frame->u.string.len = 0;

    if (c == '"') {
        puts("string: got string open");
        replace(stack, string_contents);
        return 1;
    } else if (c == '\0') {
        fail(stack);
        return 0;
    } else {
        fail(stack);
        return 0;
    }
}

int value_maybe_string(stack_t *stack, frame_t *frame, char c) {
    puts("value_maybe_string");
    if (reap(stack)) {
        close(stack);
        return 0;
    } else {
        replace(stack, value_maybe_number);
        call(stack, number);
        return 0;
    }
}

int value(stack_t *stack, frame_t *frame, char c) {
    puts("value");

    if (is_ws(c)) {
        return 1;
    }

    replace(stack, value_maybe_string);
    call(stack, string);
    return 0;
}

void jsonex_init(stack_t *stack) {
    stack->len = 1;
    stack->frames[0].status = IN_USE;
    stack->frames[0].fn = value;
    for (int i = 1; i < STACK_FRAME_COUNT; i++) {
        stack->frames[i].status = FREE;
    }
}

int jsonex_call(stack_t *stack, char c) {
    while (stack->len > 0) {
        // A parse_fn_t should return truthy if the character was consumed,
        // falsy otherwise.
        frame_t *frame = &(stack->frames[stack->len - 1]);
        size_t old_stack_len = stack->len;
        if (frame->fn(stack, frame, c)) {
            return 1;
        }
        // Keep looping until something consumes this character! (Or there's no
        // more stack.)
    }

    // Eat up trailing whitespace.
    if (is_ws(c)) {
        return 1;
    }

    return 0;
}

const char *jsonex_success = "success";
const char *jsonex_fail = "fail";

const char *jsonex_finish(stack_t *stack) {
    // All parse functions should complete() or abort() when given '\0', so
    // each time we call_stack(.., '\0') there should be one less frame.
    while (stack->len > 0) {
        size_t old_stack_len = stack->len;
        printf("finishing, c = '\\0'\n");
        if (jsonex_call(stack, '\0')) {
            return "internal error: some parse function consumed '\\0'";
        }
        if (stack->len >= old_stack_len) {
            return "internal error: stack->len not decreasing";
        }
    }

    // Since init_stack() put something on the stack, and there was nothing
    // left to reap() it, we ought to have a zombie left over telling us
    // whether the parse succeeded or failed.
    if (stack->frames[0].status != ZOMBIE) {
        return "internal error: first frame isn't a zombie";
    }
    if (stack->frames[0].is_complete) {
        return jsonex_success;
    }
    return jsonex_fail;
}

const char *feed(char *s, size_t len) {
    stack_t stack;
    jsonex_init(&stack);

    for (int i = 0; i < len; i++) {
        printf("c = %c\n", s[i]);
        if (!jsonex_call(&stack, s[i])) {
            return jsonex_fail;
        }
    }

    return jsonex_finish(&stack);
}

int main(void) {
    char *json = "  [ 1 , 2 , true , false , null , 4 , { \"blah\" : null } ] ";//"[1,2,{\"1\":{\"2\":3}}]";//"{\"a\":123.456,\"xyz\":\"qwerty\",\"1\":{\"2\":3}}"; //"494.123"; //"\"ass\"";
    puts(feed(json, strlen(json)));
}
