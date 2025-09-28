#include "cliargs.h"
#include "jp.h"
#include <stdlib.h>

typedef struct {
    const char *flag;
    const char *val;
    u16 flag_len;
    u16 val_len;
    u16 prefix_dash_count;
} cliargs_arg;

cliargs_error
cliargs_parse_value(cliargs_type t, const char *arg, cliargs_val *value) {
    cliargs_val v;

    switch (t) {
    case cliargs_type_bool:
        if (jp_cstr_eq_unsafe(arg, "true")) {
            v.uint = 1;
        } else if (jp_cstr_eq_unsafe(arg, "false")) {
            v.uint = 0;
        } else {
            return cliargs_error_parse_fail;
        }
        *value = v;
        return cliargs_error_none;
    case cliargs_type_f64:
        v.real = strtod(arg, NULL);
        *value = v;
        return cliargs_error_none;
    case cliargs_type_s64:
        v.sint = strtoll(arg, NULL, 10);
        *value = v;
        return cliargs_error_none;
    case cliargs_type_u64:
        v.uint = strtoull(arg, NULL, 10);
        *value = v;
        return cliargs_error_none;
    case cliargs_type_str:
        v.str = arg;
        *value = v;
        return cliargs_error_none;
    default: return cliargs_error_unknown_type;
    }
}

cliargs_arg cliargs_parse_arg(const char *arg) {
    cliargs_arg a = {0};
    int parsing_mode = 1, previous_parsing_mode = 1;
    u16 i = 0;

    while (arg[i]) {
        previous_parsing_mode = parsing_mode;
        switch (parsing_mode) {
        case 1: // flag or val
            for (; arg[i] && parsing_mode == previous_parsing_mode; i += 1) {
                switch (arg[i]) {
                case '-': a.prefix_dash_count += 1; break;
                case ' ':
                case '\t':
                case '\n':
                    // skip whitespace
                    break;
                default:
                    if (a.prefix_dash_count > 0) {
                        // start parsing an flag
                        a.flag = &arg[i];
                        a.flag_len += 1;
                        parsing_mode = 2;
                    } else {
                        // start parsing a val
                        a.val = &arg[i];
                        a.val_len += 1;
                        parsing_mode = 3;
                    }
                    break;
                }
            }
            break;
        case 2: // flag
            for (; arg[i] && parsing_mode == previous_parsing_mode; i += 1) {
                switch (arg[i]) {
                case '=':
                    if (a.flag_len == 0) {
                        // dashes followed by = means there's no flag
                        a.val = arg;
                        a.val_len = i;
                    }
                    parsing_mode = 3;
                    break;
                default:
                    if (!a.flag) {
                        a.flag = &arg[i];
                    }
                    a.flag_len += 1;
                    break;
                }
            }
            break;
        case 3: // val
            for (; arg[i] && parsing_mode == previous_parsing_mode; i += 1) {
                if (!a.val) {
                    a.val = &arg[i];
                }
                a.val_len += 1;
            }
            break;
        case 0:
            // nothing left to parse, exit
            break;
        default: assert(0 && "unknown parsing mode"); break;
        }
    }

    return a;
}

cliargs_opt *
cliargs_find_by_name(cliargs *args, const char *name, size_t name_len) {
    assert(args && "args must not be null");

    for (size_t i = 0; i < args->named.len; i += 1) {
        cliargs_opt *opt = &args->named.opts[i];
        size_t opt_short_name_len = jp_cstr_len_unsafe(opt->short_name);
        size_t opt_long_name_len = jp_cstr_len_unsafe(opt->long_name);
        b32 found =
            jp_cstr_eq(opt->short_name, name, min(opt_short_name_len, name_len))
            || jp_cstr_eq(
                opt->long_name, name, min(opt_long_name_len, name_len)
            );
        if (found) {
            return opt;
        }
    }
    return NULL;
}

cliargs_error cliargs_add_pos_arg(cliargs *args, const char *arg) {
    if (args->positional.len >= args->positional.max_len) {
        return cliargs_error_too_many_pos_args;
    }
    args->positional.vals[args->positional.len] = arg;
    args->positional.len += 1;
    return cliargs_error_none;
}

cliargs_error cliargs_parse_value_to_opt(cliargs_opt *opt, const char *value) {
    if (!opt) {
        return cliargs_error_unknown_flag;
    }
    if (opt->len >= opt->max_len) {
        return cliargs_error_too_many_flag_args;
    }
    cliargs_error parse_res =
        cliargs_parse_value(opt->type, value, (opt->vals + opt->len));
    if (parse_res) {
        return parse_res;
    };
    opt->len += 1;
    return cliargs_error_none;
}

cliargs_error cliargs_opt_add_value(cliargs_opt *opt, cliargs_val v) {
    if (opt->len >= opt->max_len) {
        return cliargs_error_too_many_flag_args;
    }
    opt->vals[opt->len] = v;
    opt->len += 1;
    return cliargs_error_none;
}

cliargs_error cliargs_parse(cliargs *args, int argc, char **argv) {
    assert(args && "args must not be null");
    assert(argv && "argv must not be null");

    b32 positional_only = 0;
    cliargs_opt *opt = NULL;

    for (int i = 0; i < argc; i += 1) {
        const char *arg = argv[i];
        if (!arg) {
            // skip over null args
            continue;
        }

        if (positional_only) {
            // skip over flag usage when only positionals are expected
            cliargs_error err = cliargs_add_pos_arg(args, arg);
            if (err) {
                return err;
            }
            continue;
        }

        cliargs_arg a = cliargs_parse_arg(arg);
        if (opt) {
            // awaiting val for an opt
            if (!a.val_len) {
                return cliargs_error_value_expected;
            }
            cliargs_error err = cliargs_parse_value_to_opt(opt, a.val);
            if (err) {
                return err;
            }
            opt = NULL;
        } else if (a.flag_len == 0 && a.val_len == 0
                   && a.prefix_dash_count > 1) {
            // multiple prefix dashes = rest of the args are positional
            positional_only = 1;
        } else if (a.flag_len > 0 && a.val_len > 0) {
            // flag and val combined
            opt = cliargs_find_by_name(args, a.flag, a.flag_len);
            cliargs_error err = cliargs_parse_value_to_opt(opt, a.val);
            if (err) {
                return err;
            }
            opt = NULL;
        } else if (a.flag_len > 0) {
            // only flag found
            opt = cliargs_find_by_name(args, a.flag, a.flag_len);
            if (!opt) {
                return cliargs_error_unknown_flag;
            }
            if (opt->type == cliargs_type_bool) {
                // boolean flags don't need to wait for a val
                cliargs_val v = {.uint = 1};
                cliargs_error err = cliargs_opt_add_value(opt, v);
                if (err) {
                    return err;
                }
                opt = NULL;
            }
        } else {
            cliargs_error err = cliargs_add_pos_arg(args, arg);
            if (err) {
                return err;
            }
        }
    }

    return cliargs_error_none;
}
