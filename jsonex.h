#ifndef __JSONEX_H__
#define __JSONEX_H__

#define CONTEXT_FRAME_COUNT 16
#define MAX_STRING_SIZE 64

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
    int found;
} jsonex_rule_t;

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
        int boolean;
    } u;
    parse_fn_t fn;
    int is_complete;
    jsonex_type_t type;
} frame_t;

typedef struct context {
    frame_t frames[CONTEXT_FRAME_COUNT];
    size_t frames_len;
    char paths[CONTEXT_FRAME_COUNT][MAX_STRING_SIZE];
    size_t paths_len;
    jsonex_rule_t *rules;
    const char *error;
} context_t;

void jsonex_init(struct context *, jsonex_rule_t *);
int jsonex_call(struct context *, char);
const char *jsonex_finish(struct context *);

#endif
