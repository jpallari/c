#ifndef CLIARGS_H
#define CLIARGS_H

#include "jp.h"

typedef enum {
    cliargs_error_none = 0,
    cliargs_error_parse_fail,
    cliargs_error_too_many_flag_args,
    cliargs_error_too_many_pos_args,
    cliargs_error_unknown_flag,
    cliargs_error_unknown_type,
    cliargs_error_value_expected,
} cliargs_error;

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

cliargs_error cliargs_parse(cliargs *args, int argc, char **argv);

#endif
