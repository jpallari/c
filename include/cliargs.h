#ifndef CLIARGS_H
#define CLIARGS_H

#include "jp.h"

typedef enum {
    cliargs_type_s64,
    cliargs_type_u64,
    cliargs_type_f64,
    cliargs_type_str,
    cliargs_type_bool,
} cliargs_type;

typedef union {
    s64 sint;
    u64 uint;
    double real;
    const char *str;
} cliargs_val;

typedef struct {
    const char *long_name;
    const char *short_name;
    const char *help_text;
    cliargs_val *vals;
    u32 len;
    u32 max_len;
    cliargs_type type;
} cliargs_opt;

typedef struct {
    struct {
        cliargs_opt *opts;
        u32 len;
    } named;
    struct {
        char const **vals;
        u32 len;
        u32 max_len;
    } positional;
    struct {
        char *buffer;
        u32 len;
        u32 max_len;
    } errors;
} cliargs;

#define cliargs_opt_add(opts, count, lname, sname, htext, vs, t) \
    (opts)[(count)++] = (cliargs_opt) { \
        .long_name = (lname), \
        .short_name = (sname), \
        .help_text = (htext), \
        .vals = (cliargs_val *)(vs), \
        .len = 0, \
        .max_len = jp_countof(vs), \
        .type = (t) \
    }

b32 cliargs_parse(cliargs *args, int argc, char **argv);

#endif
