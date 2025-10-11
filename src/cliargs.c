#include "cliargs.h"
#include "std.h"
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    cliargs *args;
    char **argv;
    u32 index;
} cliargs_state;

typedef struct {
    const char *flag;
    const char *val;
    u16 flag_len;
    u16 val_len;
    u16 prefix_dash_count;
} cliargs_arg;

const char *cliargs_type_to_name(cliargs_type type) {
    switch (type) {
    case cliargs_type_s64: return cliargs_type_s64_tname;
    case cliargs_type_u64: return cliargs_type_u64_tname;
    case cliargs_type_f64: return cliargs_type_f64_tname;
    case cliargs_type_str: return cliargs_type_str_tname;
    case cliargs_type_bool: return cliargs_type_bool_tname;
    default: assert(0 && "unexpected type"); return "";
    }
}

void cliargs_write_error(cliargs_state state, const char *format, ...) {
    assert(
        (!state.args->errors.max_len || state.args->errors.buffer)
        && "errors buffer must not be null"
    );

    if (state.args->errors.len >= state.args->errors.max_len) {
        return;
    }

    size_t remaining_len = state.args->errors.max_len - state.args->errors.len;
    int len = 0;

    len = snprintf(
        state.args->errors.buffer + state.args->errors.len,
        remaining_len,
        "Failed to process argument #%d '%s': ",
        state.index + 1,
        state.argv[state.index]
    );
    if (len < 0) {
        return;
    }
    state.args->errors.len += (u32)len;

    va_list va_args;
    va_start(va_args, format);
    len = vsnprintf(
        state.args->errors.buffer + state.args->errors.len,
        remaining_len,
        format,
        va_args
    );
    va_end(va_args);
    if (len < 0) {
        return;
    }

    state.args->errors.len += (u32)len;
    u32 last_index =
        min(state.args->errors.len, state.args->errors.max_len - 1);
    state.args->errors.buffer[last_index] = '\0';
}

cliargs_error cliargs_parse_value(
    cliargs_type t, const char *arg, size_t arg_len, cliargs_val *value
) {
    assert(arg && "arg must not be null");
    assert(value && "value must not be null");
    b32 ok = 0;

    switch (t) {
    case cliargs_type_bool:
        if (cstr_eq(arg, "true", arg_len)) {
            value->uint = 1;
        } else if (cstr_eq(arg, "false", arg_len)) {
            value->uint = 0;
        } else {
            return cliargs_error_parse_fail;
        }
        return cliargs_error_none;
    case cliargs_type_f64:
        ok = cstr_to_double(arg, arg_len, &value->real);
        if (!ok) {
            return cliargs_error_parse_fail;
        }
        return cliargs_error_none;
    case cliargs_type_s64:
        ok = cstr_to_s64(arg, arg_len, &value->sint);
        if (!ok) {
            return cliargs_error_parse_fail;
        }
        return cliargs_error_none;
    case cliargs_type_u64:
        ok = cstr_to_u64(arg, arg_len, &value->uint);
        if (!ok) {
            return cliargs_error_parse_fail;
        }
        return cliargs_error_none;
    case cliargs_type_str:
        value->str = arg;
        return cliargs_error_none;
    default: return cliargs_error_unknown_type;
    }
}

cliargs_arg cliargs_parse_arg(const char *arg) {
    assert(arg && "arg must not be null");

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
cliargs_find_by_name(cliargs_state state, const char *name, size_t name_len) {
    assert(state.args && "args must not be null");

    for (size_t i = 0; i < state.args->named.len; i += 1) {
        cliargs_opt *opt = &state.args->named.opts[i];
        size_t opt_short_name_len = cstr_len_unsafe(opt->short_name);
        size_t opt_long_name_len = cstr_len_unsafe(opt->long_name);
        b32 found =
            cstr_eq(opt->short_name, name, min(opt_short_name_len, name_len))
            || cstr_eq(opt->long_name, name, min(opt_long_name_len, name_len));
        if (found) {
            return opt;
        }
    }

    cliargs_write_error(state, "Unknown flag.");
    return NULL;
}

cliargs_error cliargs_add_pos_arg(cliargs_state state) {
    assert(state.args && "args must not be null");
    const char *arg = state.argv[state.index];

    if (state.args->positional.len >= state.args->positional.max_len) {
        cliargs_write_error(
            state,
            "Too many positional arguments. Expected max %d arguments.",
            state.args->positional.max_len
        );
        return cliargs_error_too_many_pos_args;
    }
    state.args->positional.vals[state.args->positional.len] = arg;
    state.args->positional.len += 1;
    return cliargs_error_none;
}

cliargs_error cliargs_parse_value_to_opt(
    cliargs_state state, cliargs_opt *opt, const char *arg, size_t arg_len
) {
    assert(state.args && "args must not be null");
    assert(opt && "opt must not be null");

    if (opt->len >= opt->max_len) {
        cliargs_write_error(
            state,
            "Too many arguments for '%s'. Expected max %d arguments.",
            opt->long_name,
            opt->max_len
        );
        return cliargs_error_too_many_flag_args;
    }

    cliargs_error err =
        cliargs_parse_value(opt->type, arg, arg_len, (opt->vals + opt->len));

    switch (err) {
    case cliargs_error_none: break;
    case cliargs_error_parse_fail:
        cliargs_write_error(
            state,
            "Could not parse value of type %s.",
            cliargs_type_to_name(opt->type)
        );
        return err;
    case cliargs_error_unknown_type:
        cliargs_write_error(state, "Unexpected type.");
        return err;
    case cliargs_error_unknown_flag:
    case cliargs_error_too_many_flag_args:
    case cliargs_error_too_many_pos_args:
    case cliargs_error_value_expected:
    default: assert(0 && "unexpected error type"); return err;
    }

    opt->len += 1;
    return cliargs_error_none;
}

cliargs_error
cliargs_opt_add_value(cliargs_state state, cliargs_opt *opt, cliargs_val v) {
    assert(state.args && "args must not be null");

    if (opt->len >= opt->max_len) {
        cliargs_write_error(
            state,
            "Too many arguments for '%s'. Expected max %d arguments.",
            opt->long_name,
            opt->max_len
        );
        return cliargs_error_too_many_flag_args;
    }
    opt->vals[opt->len] = v;
    opt->len += 1;
    return cliargs_error_none;
}

cliargs_error cliargs_parse(cliargs *args, int argc, char **argv) {
    assert(args && "args must not be null");
    assert(argv && "argv must not be null");

    if (argc < 0) {
        // nothing to parse
        return cliargs_error_none;
    }

    u32 argc_uint = (u32)argc;
    b32 positional_only = 0;
    cliargs_state state = {
        .args = args,
        .argv = argv,
        .index = 0,
    };
    cliargs_opt *opt = NULL;

    for (; state.index < argc_uint; state.index += 1) {
        const char *arg = argv[state.index];
        if (!arg) {
            // skip over null args
            continue;
        }

        if (positional_only) {
            // skip over flag usage when only positionals are expected
            cliargs_error err = cliargs_add_pos_arg(state);
            if (err) {
                return err;
            }
            continue;
        }

        cliargs_arg a = cliargs_parse_arg(arg);
        if (opt) {
            // awaiting val for an opt
            if (!a.val_len) {
                cliargs_write_error(
                    state, "Expected a value for option '%s'.", opt->long_name
                );
                return cliargs_error_value_expected;
            }
            cliargs_error err =
                cliargs_parse_value_to_opt(state, opt, a.val, a.val_len);
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
            opt = cliargs_find_by_name(state, a.flag, a.flag_len);
            if (!opt) {
                return cliargs_error_unknown_flag;
            }

            cliargs_error err =
                cliargs_parse_value_to_opt(state, opt, a.val, a.val_len);
            if (err) {
                return err;
            }
            opt = NULL;
        } else if (a.flag_len > 0) {
            // only flag found
            opt = cliargs_find_by_name(state, a.flag, a.flag_len);
            if (!opt) {
                return cliargs_error_unknown_flag;
            }
            if (opt->type == cliargs_type_bool) {
                // boolean flags don't need to wait for a val
                cliargs_val v = {.uint = 1};
                cliargs_error err = cliargs_opt_add_value(state, opt, v);
                if (err) {
                    return err;
                }
                opt = NULL;
            }
        } else {
            cliargs_error err = cliargs_add_pos_arg(state);
            if (err) {
                return err;
            }
        }
    }

    return cliargs_error_none;
}

void cliargs_init(
    cliargs *args,
    cliargs_opt *named_opts_storage,
    u32 named_opts_max_len,
    char const **pos_args_storage,
    u32 pos_args_max_len,
    char *errors_buffer,
    u32 errors_max_len
) {
    assert(args && "args must not be null");
    assert(
        (!named_opts_max_len || named_opts_storage)
        && "named opts must not be null"
    );
    assert(
        (!pos_args_max_len || pos_args_storage) && "pos args must not be null"
    );
    assert((!errors_max_len || errors_buffer) && "named opts must not be null");

    bytes_set(args, 0, sizeof(*args));
    args->named.len = 0;
    args->named.max_len = named_opts_max_len;
    args->named.opts = named_opts_storage;
    args->positional.len = 0;
    args->positional.max_len = pos_args_max_len;
    args->positional.vals = pos_args_storage;
    args->errors.len = 0;
    args->errors.max_len = errors_max_len;
    args->errors.buffer = errors_buffer;
}

u32 *cliargs_add_named(cliargs *args, cliargs_opt_spec opt_spec, void *vals) {
    assert(args && "args must not be null");

    if (args->named.len >= args->named.max_len) {
        // no space left for more named args
        return NULL;
    }

    cliargs_opt *opt = &args->named.opts[args->named.len];
    args->named.len += 1;

    opt->long_name = opt_spec.long_name;
    opt->short_name = opt_spec.short_name;
    opt->help_text = opt_spec.help_text;
    opt->type = opt_spec.type;
    opt->len = 0;
    opt->max_len = opt_spec.max_len;
    opt->vals = (cliargs_val *)vals;

    return &opt->len;
}
