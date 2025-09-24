#include "cliargs.h"

cliargs_arg parse_arg(const char *arg) {
    cliargs_arg a = {0};
    int parsing_mode = 1, previous_parsing_mode = 1;
    u16 i = 0;

    while (arg[i]) {
        previous_parsing_mode = parsing_mode;
        switch (parsing_mode) {
        case 1: // opt or val
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
                        // start parsing an opt
                        a.opt = &arg[i];
                        a.opt_len += 1;
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
        case 2: // opt
            for (; arg[i] && parsing_mode == previous_parsing_mode; i += 1) {
                switch (arg[i]) {
                case '=':
                    if (a.opt_len == 0) {
                        // dashes followed by = means there's no opt
                        a.val = arg;
                        a.val_len = i;
                    }
                    parsing_mode = 3;
                    break;
                default:
                    if (!a.opt) {
                        a.opt = &arg[i];
                    }
                    a.opt_len += 1;
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
                // contains only two or more dashes? rest of the args are
                // positional only check if name exists in opts... no? raise
                // error!
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
