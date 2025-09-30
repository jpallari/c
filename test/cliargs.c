#include "cliargs.h"
#include "jp.h"
#include "testr.h"

void parse_opts_all(test *t) {
    cliargs_opt named_opts[5] = {0};
    char const *positional_vals[10] = {0};
    char errors[2048] = {0};
    cliargs args = {0};

    cliargs_init(
        &args,
        named_opts,
        jp_countof(named_opts),
        positional_vals,
        jp_countof(positional_vals),
        errors,
        jp_countof(errors)
    );

    u64 flag_help[1] = {0};
    u32 *flag_help_len = cliargs_add_named(
        &args,
        (cliargs_opt_spec) {
            .long_name = "help",
            .short_name = "h",
            .help_text = "Display help",
            .max_len = jp_countof(flag_help),
            .type = cliargs_type_bool,

        },
        flag_help
    );

    char *flag_greeting[3] = {0};
    u32 *flag_greeting_len = cliargs_add_named(
        &args,
        (cliargs_opt_spec) {
            .long_name = "greeting",
            .short_name = "g",
            .help_text = "Greeting message",
            .max_len = jp_countof(flag_greeting),
            .type = cliargs_type_str,

        },
        flag_greeting
    );

    double flag_percentage[1] = {0};
    u32 *flag_percentage_len = cliargs_add_named(
        &args,
        (cliargs_opt_spec) {
            .long_name = "percentage",
            .short_name = "p",
            .help_text = "Percentage number between 0..1",
            .max_len = jp_countof(flag_percentage),
            .type = cliargs_type_f64,
        },
        flag_percentage
    );

    u64 flag_count[4] = {0};
    u32 *flag_count_len = cliargs_add_named(
        &args,
        (cliargs_opt_spec) {
            .long_name = "count",
            .short_name = "c",
            .help_text = "Count of things",
            .max_len = jp_countof(flag_count),
            .type = cliargs_type_u64,
        },
        flag_count
    );

    s64 flag_diff[1] = {0};
    u32 *flag_diff_len = cliargs_add_named(
        &args,
        (cliargs_opt_spec) {
            .long_name = "diff",
            .short_name = "d",
            .help_text = "Difference",
            .max_len = jp_countof(flag_diff),
            .type = cliargs_type_s64,

        },
        flag_diff
    );

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
            cliargs_parse(&args, jp_countof(argv), argv),
            "cli args must be parsed w/o errors"
        )) {
        return;
    }

    // lengths
    assert_eq_uint(t, *flag_help_len, 1, "help length");
    assert_eq_uint(t, *flag_greeting_len, 2, "greeting length");
    assert_eq_uint(t, *flag_percentage_len, 1, "percentage length");
    assert_eq_uint(t, *flag_count_len, 2, "count length");
    assert_eq_uint(t, *flag_diff_len, 1, "diff length");
    assert_eq_uint(t, args.positional.len, 2, "positional length");
    assert_eq_uint(t, args.errors.len, 0, "errors length");

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
