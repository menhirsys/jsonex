#ifndef __JSONEX_H__
#define __JSONEX_H__

#include <stddef.h>

#define JSONEX_MAX_STRING_SIZE 64
#define JSONEX_CONTEXT_FRAME_COUNT 16

typedef enum {
    JSONEX_INTEGER,
    JSONEX_STRING,
    JSONEX_BOOL,
    JSONEX_NONE
} jsonex_type_t;

typedef struct {
    jsonex_type_t type;
    void *p;
    char **path;
    int *found;
} jsonex_rule_t;

struct jsonex_context;
struct jsonex_frame;

typedef int (*parse_fn_t)(struct jsonex_context *, struct jsonex_frame *, char);

typedef struct jsonex_frame {
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
        char string[JSONEX_MAX_STRING_SIZE];
        int boolean;
    } u;
    parse_fn_t fn;
    int is_complete;
    jsonex_type_t type;
} jsonex_frame_t;

typedef struct jsonex_context {
    jsonex_frame_t frames[JSONEX_CONTEXT_FRAME_COUNT];
    size_t frames_len;
    char paths[JSONEX_CONTEXT_FRAME_COUNT][JSONEX_MAX_STRING_SIZE];
    size_t paths_len;
    jsonex_rule_t *rules;
    const char *error;
} jsonex_context_t;

void jsonex_init(jsonex_context_t *, jsonex_rule_t *);
int jsonex_call(jsonex_context_t *, char);
const char *jsonex_finish(jsonex_context_t *);

#endif
