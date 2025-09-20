#ifndef JP_TEST_H
#define JP_TEST_H

#include "jp.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

////////////////////////
// Config
////////////////////////

static int color_enabled = 1;
static int trap_on_assert_fail = 0;

////////////////////////
// Logging
////////////////////////

enum log_status {
    log_status_test,
    log_status_ok,
    log_status_warn,
    log_status_fail,
};

void log_no_loc(enum log_status status, const char *msg) {
    char *status_str = "";
    char *color_start = "";
    char *color_end = "";

    switch (status) {
    case log_status_test: status_str = "TEST"; break;
    case log_status_ok: status_str = "OK  "; break;
    case log_status_warn: status_str = "WARN"; break;
    case log_status_fail: status_str = "FAIL"; break;
    }
    if (color_enabled) {
        switch (status) {
        case log_status_test: color_start = "\x1B[36m"; break;
        case log_status_ok: color_start = "\x1B[32m"; break;
        case log_status_warn: color_start = "\x1B[33m"; break;
        case log_status_fail: color_start = "\x1B[31m"; break;
        }
        color_end = "\x1B[0m";
    }

    fprintf(stderr, "%s[ %s ]%s %s\n", color_start, status_str, color_end, msg);
}

void log_with_loc(
    enum log_status status,
    const char *file,
    int line,
    const char *func,
    const char *msg
) {
    char *status_str = "";
    char *color_start = "";
    char *color_end = "";

    switch (status) {
    case log_status_test: status_str = "TEST"; break;
    case log_status_ok: status_str = "OK  "; break;
    case log_status_warn: status_str = "WARN"; break;
    case log_status_fail: status_str = "FAIL"; break;
    }
    if (color_enabled) {
        switch (status) {
        case log_status_test: color_start = "\x1B[36m"; break;
        case log_status_ok: color_start = "\x1B[32m"; break;
        case log_status_warn: color_start = "\x1B[33m"; break;
        case log_status_fail: color_start = "\x1B[31m"; break;
        }
        color_end = "\x1B[0m";
    }

    fprintf(
        stderr,
        "%s[ %s ]%s %s:%d (%s): %s\n",
        color_start,
        status_str,
        color_end,
        file,
        line,
        func,
        msg
    );
}

#define log(status, msg) \
    log_with_loc((status), __FILE__, __LINE__, __func__, (msg))
#define log_test(msg) log(log_status_test, msg)
#define log_ok(msg) log(log_status_ok, msg)
#define log_warn(msg) log(log_status_warn, msg)
#define log_fail(msg) log(log_status_fail, msg)

////////////////////////
// Assertions
////////////////////////

#define __assert_msg_buf_size 1024
#define breakpoint __asm__("int3; nop")

#define __assert_fail_inc fails += 1
#define __assert_fail_bail return 1

#define __assert_fail(msg) \
    do { \
        log_fail(msg); \
        if (trap_on_assert_fail) { \
            breakpoint; \
        } \
    } while (0)

#define __assert_cmp_log_msg(buf, l, r, cmp, f, msg) \
    snprintf((buf), sizeof(buf), (f " " #cmp " " f " // %s"), (l), (r), (msg))

#define __assert_cmp_base(c, l, r, cmp, f, msg, action) \
    do { \
        if (!(c)) { \
            char __assert_log_msg[1024] = {0}; \
            snprintf( \
                __assert_log_msg, \
                sizeof(__assert_log_msg), \
                (f " " #cmp " " f " // %s"), \
                (l), \
                (r), \
                (msg) \
            ); \
            __assert_fail(__assert_log_msg); \
            action; \
        } \
    } while (0)

#define __assert_cmp_base_simple(l, r, cmp, f, msg, action) \
    __assert_cmp_base((l)cmp(r), l, r, cmp, f, msg, action)

#define assert_false_inc(c, msg) \
    do { \
        if (c) { \
            __assert_fail(msg); \
            __assert_fail_inc; \
        } \
    } while (0)

#define assert_false(c, msg) \
    do { \
        if (c) { \
            __assert_fail(msg); \
            __assert_fail_bail; \
        } \
    } while (0)

#define assert_cmp(l, r, cmp, f, msg) \
    __assert_cmp_base_simple(l, r, cmp, f, msg, return 1)

#define assert_cmp_inc(l, r, cmp, f, msg) \
    __assert_cmp_base_simple(l, r, cmp, f, msg, fails += 1)

#define assert_eq_cstr(l, r, msg) \
    __assert_cmp_base( \
        jp_cstr_eq_unsafe((l), (r)), l, r, ==, "%s", msg, __assert_fail_bail \
    )

#define assert_eq_cstr_inc(l, r, msg) \
    __assert_cmp_base( \
        jp_cstr_eq_unsafe((l), (r)), ==, "%s", msg, __assert_fail_inc \
    )

#define assert_ne_cstr(l, r, msg) \
    __assert_cmp_base( \
        !jp_cstr_eq_unsafe((l), (r)), l, r, ==, "%s", msg, __assert_fail_bail \
    )

#define assert_ne_cstr_inc(l, r, msg) \
    __assert_cmp_base( \
        !jp_cstr_eq_unsafe((l), (r)), l, r, ==, "%s", msg, __assert_fail_inc \
    )

#define assert_true(c, msg) assert_false(!(c), msg)
#define assert_eq(a, b, f, msg) assert_cmp(a, b, ==, f, msg)
#define assert_ne(a, b, f, msg) assert_cmp(a, b, !=, f, msg)
#define assert_lt(a, b, f, msg) assert_cmp(a, b, <, f, msg)
#define assert_le(a, b, f, msg) assert_cmp(a, b, <=, f, msg)
#define assert_gt(a, b, f, msg) assert_cmp(a, b, >, f, msg)
#define assert_ge(a, b, f, msg) assert_cmp(a, b, >=, f, msg)
#define assert_true_inc(c, msg) assert_false_inc(!(c), msg)
#define assert_eq_inc(a, b, f, msg) assert_cmp_inc(a, b, ==, f, msg)
#define assert_ne_inc(a, b, f, msg) assert_cmp_inc(a, b, !=, f, msg)
#define assert_lt_inc(a, b, f, msg) assert_cmp_inc(a, b, <, f, msg)
#define assert_le_inc(a, b, f, msg) assert_cmp_inc(a, b, <=, f, msg)
#define assert_gt_inc(a, b, f, msg) assert_cmp_inc(a, b, >, f, msg)
#define assert_ge_inc(a, b, f, msg) assert_cmp_inc(a, b, >=, f, msg)

////////////////////////
// Test runner
////////////////////////

typedef int (*test_case_fun)(void);

typedef struct {
    void (*before_all)(void);
    void (*before)(void);
    void (*after)(void);
    void (*after_all)(void);
} test_setup;

typedef struct {
    char *name;
    test_case_fun test_code;
} test_case;

int test_main(
    int argc,
    char **argv,
    test_setup *setup,
    size_t test_count,
    test_case *test_cases
) {
    char buf[1024] = {0};

    // settings
    const char *no_color = getenv("NO_COLOR");
    if (no_color) {
        color_enabled = 0;
    }
    const char *assert_trap = getenv("ASSERT_TRAP");
    if (assert_trap) {
        trap_on_assert_fail = 1;
    }

    if (!test_cases) {
        log_fail("No test cases defined");
        return 1;
    }

    int failed_test_cases = 0;
    int failed_asserts = 0;
    int test_case_resp = 0;
    test_case test_case;

    if (setup && setup->before_all) {
        setup->before_all();
    }
    for (size_t i = 0; i < test_count; i += 1) {
        test_case = test_cases[i];
        log_no_loc(log_status_test, test_case.name);
        if (setup && setup->before) {
            setup->before();
        }
        test_case_resp = test_case.test_code();
        failed_asserts += test_case_resp;
        if (test_case_resp > 0) {
            snprintf(
                buf,
                sizeof(buf),
                "%s failed %d asserts",
                test_case.name,
                test_case_resp
            );
            log_no_loc(log_status_fail, buf);
            failed_test_cases += 1;
        } else {
            log_no_loc(log_status_ok, test_case.name);
        }
        if (setup && setup->after) {
            setup->after();
        }
        fprintf(stderr, "\n");
    }
    if (setup && setup->after_all) {
        setup->after_all();
    }

    enum log_status final_status;
    if (failed_test_cases) {
        final_status = log_status_fail;
        snprintf(
            buf,
            sizeof(buf),
            "%d/%ld test cases failed with %d failed asserts",
            failed_test_cases,
            test_count,
            failed_asserts
        );
    } else {
        final_status = log_status_ok;
        snprintf(buf, sizeof(buf), "all tests pass");
    }
    fprintf(stderr, "----------------------------------------\n");
    log_no_loc(final_status, buf);
    return failed_test_cases;
}

#define setup_tests(setup, tests) \
    int main(int argc, char **argv) { \
        return test_main( \
            argc, argv, (setup), sizeof(tests) / sizeof(*tests), (tests) \
        ); \
    }

#endif
