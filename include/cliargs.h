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

typedef struct {
    u64 value;
    u32 count;
    u32 max_count;
    cliargs_type t;
} cliargs_val;

typedef struct {
    char *long_name;
    char *short_name;
    char *help_text;
} cliargs_opt;

typedef struct {
    struct {
        cliargs_opt opt;
        cliargs_val *val;
    } named;
    struct {
        cliargs_val *val;
        u32 count;
        u32 max_count;
    } positional;
    char *errors;
    u32 errors_size;
} cliargs_opts;

typedef struct {
    const char *opt;
    const char *val;
    u16 opt_len;
    u16 val_len;
    u16 prefix_dash_count;
} cliargs_arg;

cliargs_arg parse_arg(const char *arg);

b32 parse(cliargs_opts *opts, int argc, char **argv);

#endif

