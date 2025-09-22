#include "cliargs.h"
#include "testr.h"

void parse_single_arg(test *t) {
    // empty
    {
        cliargs_arg a = parse_arg("");
        assert_false(t, a.opt, "opt should be empty");
        assert_false(t, a.opt_len, "opt len should be 0");
        assert_false(t, a.val, "val should be empty");
        assert_false(t, a.val_len, "val len should be 0");
    }

    // val
    {
        cliargs_arg a = parse_arg("hello world");
        assert_false(t, a.opt, "opt should be empty");
        assert_false(t, a.opt_len, "opt len should be 0");
        assert_eq_bytes(
            t, a.val, "hello world", 12, "val must be 'hello world'"
        );
        assert_eq(t, a.val_len, 11, "%d", "val len should be set");
    }

    // short opt name only
    {
        cliargs_arg a = parse_arg("-f");
        assert_eq_bytes(t, a.opt, "f", 1, "opt must be 'f'");
        assert_eq(t, a.opt_len, 1, "%d", "opt len should be set");
        assert_false(t, a.val, "val should be empty");
        assert_false(t, a.val_len, "val len should be 0");
    }

    // long opt name only
    {
        cliargs_arg a = parse_arg("--force");
        assert_eq_bytes(t, a.opt, "force", 5, "opt must be 'force'");
        assert_eq(t, a.opt_len, 5, "%d", "opt len should be set");
        assert_false(t, a.val, "val should be empty");
        assert_false(t, a.val_len, "val len should be 0");
    }

    // short opt with val
    {
        cliargs_arg a = parse_arg("-hello=world!");
        assert_eq_bytes(t, a.opt, "hello", 5, "opt must be 'hello'");
        assert_eq(t, a.opt_len, 5, "%d", "opt len should be set");
        assert_eq_bytes(t, a.val, "world!", 6, "val must be 'world!'");
        assert_eq(t, a.val_len, 6, "%d", "val len should be set");
    }

    // long opt with val
    {
        cliargs_arg a = parse_arg("--foo=bar");
        assert_eq_bytes(t, a.opt, "foo", 3, "opt must be 'foo'");
        assert_eq(t, a.opt_len, 3, "%d", "opt len should be set");
        assert_eq_bytes(t, a.val, "bar", 3, "val must be 'bar'");
        assert_eq(t, a.val_len, 3, "%d", "val len should be set");
    }

    // separator
    {
        cliargs_arg a = parse_arg("--");
        assert_false(t, a.opt, "opt should be empty");
        assert_false(t, a.opt_len, "opt len should be 0");
        assert_false(t, a.val, "val should be empty");
        assert_false(t, a.val_len, "val len should be 0");
        assert_eq(
            t, a.prefix_dash_count, 2, "%d", "prefix dash count must be 2"
        );
    }
}

static test_case tests[] = {{"Parse single arg", parse_single_arg}};

setup_tests(NULL, tests)
