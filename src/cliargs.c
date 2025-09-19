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

b32 parse(cliargs_opts *opts, int argc, char **argv) {
    b32 positional_only = 0;
    cliargs_opt *opt = NULL;
    cliargs_val *val = NULL;

    for (int i = 0; i < argc; i += 1) {
        char *arg = argv[i];
        if (!arg) {
            continue;
        }

        if (positional_only) {
            // positional arg
            // parse value and push it to val
        } else if (val) {
            // parse and assing to option
            // parse fail --> raise error
        } else {
            if (arg[0] == '-') {
                // named arg
                // scroll through dashes to find name
                // find position of '=' to find end of name
                // contains only two or more dashes? rest of the args are positional only
                // check if name exists in opts... no? raise error!
                // `=` exists? parse remainder and push to vals
                // parse fail --> raise error
                // no `=` but bool? set val to true
                // no `=`? save val and opt pointers
            } else {
                // positional arg
                // parse value and push it to val
                val = NULL;
                opt = NULL;
            }
        }
    }

    return 1;
}
