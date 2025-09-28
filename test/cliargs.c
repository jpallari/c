#include "cliargs.h"
#include "jp.h"
#include "testr.h"

void parse_opts_all(test *t) {
    cliargs_opt named_opts[5] = {0};
    u32 named_opts_count = 0;

    u64 flag_help[1] = {0};
    named_opts[named_opts_count] = (cliargs_opt) {
        .long_name = "help",
        .short_name = "h",
        .help_text = "Display help",
        .vals = (cliargs_val *)flag_help,
        .len = 0,
        .max_len = jp_countof(flag_help),
        .type = cliargs_type_bool,
    };
    cliargs_opt *opt_help = &named_opts[named_opts_count++];

    char *flag_greeting[3] = {0};
    named_opts[named_opts_count] = (cliargs_opt) {
        .long_name = "greeting",
        .short_name = "g",
        .help_text = "Greeting message",
        .vals = (cliargs_val *)flag_greeting,
        .len = 0,
        .max_len = jp_countof(flag_greeting),
        .type = cliargs_type_str,
    };
    cliargs_opt *opt_greeting = &named_opts[named_opts_count++];

    double flag_percentage[1] = {0};
    named_opts[named_opts_count] = (cliargs_opt) {
        .long_name = "percentage",
        .short_name = "p",
        .help_text = "Percentage number between 0..1",
        .vals = (cliargs_val *)flag_percentage,
        .len = 0,
        .max_len = jp_countof(flag_percentage),
        .type = cliargs_type_f64,
    };
    cliargs_opt *opt_percentage = &named_opts[named_opts_count++];

    u64 flag_count[4] = {0};
    named_opts[named_opts_count] = (cliargs_opt) {
        .long_name = "count",
        .short_name = "c",
        .help_text = "Count of things",
        .vals = (cliargs_val *)flag_count,
        .len = 0,
        .max_len = jp_countof(flag_count),
        .type = cliargs_type_u64,
    };
    cliargs_opt *opt_count = &named_opts[named_opts_count++];

    s64 flag_diff[1] = {0};
    named_opts[named_opts_count] = (cliargs_opt) {
        .long_name = "diff",
        .short_name = "d",
        .help_text = "Difference",
        .vals = (cliargs_val *)flag_diff,
        .len = 0,
        .max_len = jp_countof(flag_diff),
        .type = cliargs_type_s64,
    };
    cliargs_opt *opt_diff = &named_opts[named_opts_count++];

    char const *positional_vals[10] = {0};
    char errors[2048] = {0};
    cliargs opts = {
        .named =
            {
                .opts = named_opts,
                .len = jp_countof(named_opts),
            },
        .positional =
            {
                .vals = positional_vals,
                .len = 0,
                .max_len = jp_countof(positional_vals),
            },
        .errors = {
            .buffer = errors,
            .len = 0,
            .max_len = jp_countof(errors),
        },
    };

    char argv_str[] =
        "--help -g hello -c 2 -p=0.2 pos1 --greeting world --count=5 -diff=-4 "
        "pos2 pos3";
    char *argv[12] = {0};
    jp_cstr_split_iter split = {
        .str = argv_str,
        .str_len = jp_countof(argv_str),
        .split_chars = " ",
        .split_chars_len = 1,
        .index = 0,
        .null_terminate = 1,
    };
    size_t argc = jp_cstr_split_collect_strings(argv, jp_countof(argv), &split);
    size_t expected_argc = jp_countof(argv);
    if (!assert_eq_uint(t, argc, expected_argc, "must parse enough args")) {
        return;
    }

    if (!assert_false(
            t,
            cliargs_parse(&opts, jp_countof(argv), argv),
            "cli args must be parsed w/o errors"
        )) {
        return;
    }

    // lengths
    assert_eq_uint(t, opt_help->len, 1, "help length");
    assert_eq_uint(t, opt_greeting->len, 2, "greeting length");
    assert_eq_uint(t, opt_percentage->len, 1, "percentage length");
    assert_eq_uint(t, opt_count->len, 2, "count length");
    assert_eq_uint(t, opt_diff->len, 1, "diff length");
    assert_eq_uint(t, opts.positional.len, 2, "positional length");
    assert_eq_uint(t, opts.errors.len, 0, "errors length");

    // values
    assert_true(t, flag_help[0], "help value");
    assert_eq_cstr(t, flag_greeting[0], "hello", "greeting first value");
    assert_eq_cstr(t, flag_greeting[1], "world", "greeting second value");
    assert_eq_float(t, flag_percentage[0], 0.2, 0.0001, "percentage value");
    assert_eq_uint(t, flag_count[0], 2, "count first value");
    assert_eq_uint(t, flag_count[1], 5, "count second value");
    assert_eq_sint(t, flag_diff[0], -4, "diff value");
}

static test_case tests[] = {{"Parse all options", parse_opts_all}};

setup_tests(NULL, tests)
