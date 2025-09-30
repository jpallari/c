#include "cliargs.h"
#include "jp.h"
#include "testr.h"

size_t
split_args(char *argv_str, size_t argv_str_len, char **argv, size_t argv_len) {
    jp_cstr_split_iter split = {
        .str = argv_str,
        .str_len = argv_str_len,
        .split_chars = " ",
        .split_chars_len = 1,
        .index = 0,
        .null_terminate = 1,
    };
    return jp_cstr_split_collect_strings(argv, argv_len, &split);
}

void test_parse_all_args(test *t) {
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
    char *argv[20] = {0};
    size_t argc =
        split_args(argv_str, jp_countof(argv_str), argv, jp_countof(argv));
    if (!assert_eq_uint(t, argc, 13, "must parse enough args")) {
        return;
    }

    // parse
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
    assert_eq_uint(t, args.positional.len, 3, "positional length");
    assert_eq_uint(t, args.errors.len, 0, "errors length");

    // values
    assert_true(t, flag_help[0], "help value");
    assert_eq_cstr(t, flag_greeting[0], "hello", "greeting first value");
    assert_eq_cstr(t, flag_greeting[1], "world", "greeting second value");
    assert_eq_float(t, flag_percentage[0], 0.2, 0.0001, "percentage value");
    assert_eq_uint(t, flag_count[0], 2, "count first value");
    assert_eq_uint(t, flag_count[1], 5, "count second value");
    assert_eq_sint(t, flag_diff[0], -4, "diff value");
    assert_eq_cstr(t, positional_vals[0], "pos1", "positional first value");
    assert_eq_cstr(t, positional_vals[1], "pos2", "positional second value");
    assert_eq_cstr(t, positional_vals[2], "pos3", "positional third value");
}

void test_parse_rest(test *t) {
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

    char argv_str[] =
        "-g hello pos1 --greeting world pos2 pos3 -- --greeting hi";
    char *argv[20] = {0};
    size_t argc =
        split_args(argv_str, jp_countof(argv_str), argv, jp_countof(argv));
    if (!assert_eq_uint(t, argc, 10, "must parse enough args")) {
        return;
    }

    // parse
    if (!assert_false(
            t,
            cliargs_parse(&args, jp_countof(argv), argv),
            "cli args must be parsed w/o errors"
        )) {
        return;
    }

    // lengths
    assert_eq_uint(t, *flag_greeting_len, 2, "greeting length");
    assert_eq_uint(t, args.positional.len, 5, "positional length");
    assert_eq_uint(t, args.errors.len, 0, "errors length");

    // values
    assert_eq_cstr(t, flag_greeting[0], "hello", "greeting first value");
    assert_eq_cstr(t, flag_greeting[1], "world", "greeting second value");
    assert_eq_cstr(t, positional_vals[0], "pos1", "positional first value");
    assert_eq_cstr(t, positional_vals[1], "pos2", "positional second value");
    assert_eq_cstr(t, positional_vals[2], "pos3", "positional third value");
    assert_eq_cstr(
        t, positional_vals[3], "--greeting", "positional fourth value"
    );
    assert_eq_cstr(t, positional_vals[4], "hi", "positional fifth value");
}

void test_parse_fail_on_parse(test *t) {
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

    u64 flag_count[4] = {0};
    cliargs_add_named(
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

    char argv_str[] = "-c fail";
    char *argv[20] = {0};
    split_args(argv_str, jp_countof(argv_str), argv, jp_countof(argv));
    cliargs_error err = cliargs_parse(&args, jp_countof(argv), argv);

    assert_true(t, err, "cli args must contain errors");
    assert_eq_uint(
        t, err, cliargs_error_parse_fail, "cli args must contain a parse error"
    );
    assert_eq_cstr(
        t,
        errors,
        "Failed to process argument #2 'fail': Could not parse value of type "
        "unsigned integer.",
        "parse error"
    );
}

void test_parse_fail_on_unknown_flag(test *t) {
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

    u64 flag_count[4] = {0};
    cliargs_add_named(
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

    char argv_str[] = "--not-count 3";
    char *argv[20] = {0};
    split_args(argv_str, jp_countof(argv_str), argv, jp_countof(argv));
    cliargs_error err = cliargs_parse(&args, jp_countof(argv), argv);

    assert_true(t, err, "cli args must contain errors");
    assert_eq_uint(
        t,
        err,
        cliargs_error_unknown_flag,
        "cli args must contain a parse error"
    );
    assert_eq_cstr(
        t,
        errors,
        "Failed to process argument #1 '--not-count': Unknown flag.",
        "parse error"
    );
}

void test_parse_fail_on_expecting_value(test *t) {
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

    u64 flag_count[4] = {0};
    cliargs_add_named(
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
    cliargs_add_named(
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

    char argv_str[] = "--count 3 --diff --count 1";
    char *argv[20] = {0};
    split_args(argv_str, jp_countof(argv_str), argv, jp_countof(argv));
    cliargs_error err = cliargs_parse(&args, jp_countof(argv), argv);

    assert_true(t, err, "cli args must contain errors");
    assert_eq_uint(
        t,
        err,
        cliargs_error_value_expected,
        "cli args must contain a parse error"
    );
    assert_eq_cstr(
        t,
        errors,
        "Failed to process argument #4 '--count': Expected a value for option "
        "'diff'.",
        "parse error"
    );
}

static test_case tests[] = {
    {"Parse all args", test_parse_all_args},
    {"Parse rest", test_parse_rest},
    {"Parse fail on parse", test_parse_fail_on_parse},
    {"Parse fail on unknown flag", test_parse_fail_on_unknown_flag},
    {"Parse fail on expecting a value for flag",
     test_parse_fail_on_expecting_value}
};

setup_tests(NULL, tests)
