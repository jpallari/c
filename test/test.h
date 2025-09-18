#ifndef JP_TEST_H
#define JP_TEST_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

////////////////////////
// Logging
////////////////////////

static int color_enabled = 1;

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
    case log_status_ok: status_str = " OK "; break;
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
    case log_status_ok: status_str = " OK "; break;
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

#define assert_false_inc(c, msg) \
    do { \
        if (c) { \
            log_fail(msg); \
            fails += 1; \
        } else { \
            log_ok(msg); \
        } \
    } while (0)

#define assert_false(c, msg) \
    do { \
        if (c) { \
            log_fail(msg); \
            return 1; \
        } else { \
            log_ok(msg); \
        } \
    } while (0)

#define assert_true(c, msg) assert_false(!(c), msg)
#define assert_eq(a, b, msg) assert_false((a) == (b), msg)
#define assert_ne(a, b, msg) assert_false((a) != (b), msg)
#define assert_lt(a, b, msg) assert_false((a) < (b), msg)
#define assert_le(a, b, msg) assert_false((a) <= (b), msg)
#define assert_gt(a, b, msg) assert_false((a) > (b), msg)
#define assert_ge(a, b, msg) assert_false((a) >= (b), msg)
#define assert_true_inc(c, msg) assert_false_inc(!(c), msg)
#define assert_eq_inc(a, b, msg) assert_false_inc((a) == (b), msg)
#define assert_ne_inc(a, b, msg) assert_false_inc((a) != (b), msg)
#define assert_lt_inc(a, b, msg) assert_false_inc((a) < (b), msg)
#define assert_le_inc(a, b, msg) assert_false_inc((a) <= (b), msg)
#define assert_gt_inc(a, b, msg) assert_false_inc((a) > (b), msg)
#define assert_ge_inc(a, b, msg) assert_false_inc((a) >= (b), msg)

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
    const char *no_color = getenv("NO_COLOR");
    if (no_color) {
        color_enabled = 0;
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
            failed_test_cases += 1;
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
    char buf[1024] = {0};
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
