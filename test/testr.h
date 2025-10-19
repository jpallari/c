#ifndef JP_TESTR_H
#define JP_TESTR_H

#include "std.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

////////////////////////
// Panic
////////////////////////

/**
 * Quit the program immediately
 */
#define panic() exit(10)

////////////////////////
// Config
////////////////////////

static int color_enabled = 1;
static int trap_on_assert_fail = 0;
static int print_all_asserts = 0;

////////////////////////
// Reporting
////////////////////////

typedef struct {
    ullong logs_offset;
    const char *file;
    int line;
    bool passed;
} test_assert;

typedef struct {
    uchar *logs; // dynamic array
} logs_handle;

typedef struct {
    test_assert *asserts; // dynamic array
} asserts_handle;

typedef struct {
    logs_handle *logs_handle;
    asserts_handle *asserts_handle;
    uint assert_count;
    uint asserts_passed;
} test;

typedef struct {
    const char *name;
    test_assert *asserts;
    uint assert_count;
    uint asserts_passed;
} test_report;

typedef struct {
    logs_handle *logs_handle;
    asserts_handle *asserts_handle;
    test_report *test_reports;
    uint test_count;
    uint tests_passed;
    uint assert_count;
    uint asserts_passed;
} test_suite_report;

bool test_report_append(
    test *t,
    const bool passed,
    const char *log_message,
    const size_t log_message_size,
    const char *file,
    const int line
) {
    assert(t && "test report must not be null");
    assert(t->logs_handle && "logs handle must not be null");
    assert(t->logs_handle->logs && "logs storage must not be null");

    ullong logs_offset = dynarr_len(t->logs_handle->logs);

    test_assert assert_report = {
        .logs_offset = logs_offset,
        .passed = passed,
        .file = file,
        .line = line,
    };
    test_assert *next_ar = dynarr_push_grow(
        t->asserts_handle->asserts, &assert_report, 1, test_assert
    );
    if (next_ar) {
        t->asserts_handle->asserts = next_ar;
    } else {
        panic();
    }

    uchar *next_logs = dynarr_push_grow(
        t->logs_handle->logs, log_message, log_message_size, uchar
    );
    if (next_logs) {
        t->logs_handle->logs = next_logs;
    } else {
        panic();
    }

    t->assert_count += 1;
    if (passed) {
        t->asserts_passed += 1;
    }

    if (!passed && trap_on_assert_fail) {
        breakpoint();
    }
    return passed;
}

bool test_report_append_formatted_cstr(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const char *left,
    const char *right
) {
    char buffer[1024] = {0};
    int len = snprintf(
        buffer, sizeof(buffer), "%s %s %s // %s", left, cmp, right, log_message
    );
    if (len < 0) {
        panic();
    }
    buffer[len] = '\0';
    return test_report_append(t, passed, buffer, (size_t)len + 1, file, line);
}

bool test_report_append_formatted_float(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const double left,
    const double right
) {
    char buffer[1024] = {0};
    int len = snprintf(
        buffer, sizeof(buffer), "%f %s %f // %s", left, cmp, right, log_message
    );
    if (len < 0) {
        panic();
    }
    buffer[len] = '\0';
    return test_report_append(t, passed, buffer, (size_t)len + 1, file, line);
}

bool test_report_append_formatted_int(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    int line,
    const char *cmp,
    const llong left,
    const llong right
) {
    char buffer[1024] = {0};
    int len = snprintf(
        buffer,
        sizeof(buffer),
        "%lld %s %lld // %s",
        left,
        cmp,
        right,
        log_message
    );
    if (len < 0) {
        panic();
    }
    buffer[len] = '\0';
    return test_report_append(t, passed, buffer, (size_t)len + 1, file, line);
}

bool test_report_append_formatted_uint(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    int line,
    const char *cmp,
    const ullong left,
    const ullong right
) {
    char buffer[1024] = {0};
    int len = snprintf(
        buffer,
        sizeof(buffer),
        "%llu %s %llu // %s",
        left,
        cmp,
        right,
        log_message
    );
    if (len < 0) {
        panic();
    }
    buffer[len] = '\0';
    return test_report_append(t, passed, buffer, (size_t)len + 1, file, line);
}

void test_suite_report_pretty(test_suite_report *report, FILE *stream) {
    assert(report && "test suite report must not be null");

    const char *color_ok = "\x1B[32m";
    const char *color_fail = "\x1B[31m";
    const char *color_skip = "\x1B[33m";
    const char *color_reset = "\x1B[0m";
    const char *color_info = "\x1B[1;30m";
    if (!color_enabled) {
        color_ok = "";
        color_fail = "";
        color_skip = "";
        color_reset = "";
        color_info = "";
    }

    if (!report->test_count) {
        fprintf(stream, "No tests executed");
        return;
    }

    const uchar *logs = report->logs_handle->logs;

    for (ullong i = 0; i < report->test_count; i += 1) {
        test_report tr = report->test_reports[i];
        const char *prefix = "";
        const char *color = "";
        if (tr.assert_count == 0) {
            prefix = "SKIP";
            color = color_skip;
        } else if (tr.assert_count > tr.asserts_passed) {
            prefix = "FAIL";
            color = color_fail;
        } else {
            prefix = " OK ";
            color = color_ok;
        }
        fprintf(
            stream,
            "%s[%s]%s %s %s(%d/%d passed)%s\n",
            color,
            prefix,
            color_reset,
            tr.name,
            color_info,
            tr.asserts_passed,
            tr.assert_count,
            color_reset
        );
        for (uint j = 0; j < tr.assert_count; j += 1) {
            test_assert ar = tr.asserts[j];
            if (!ar.passed || print_all_asserts) {
                fprintf(
                    stream,
                    "    %s:%d: %s\n",
                    ar.file,
                    ar.line,
                    (const unsigned char *)(&(logs[ar.logs_offset]))
                );
            }
        }
    }
}

////////////////////////
// Test runner
////////////////////////

typedef void (*test_case_fun)(test *t);

typedef struct {
    void (*before_all)(void);
    void (*before)(void);
    void (*after)(void);
    void (*after_all)(void);
} test_setup;

typedef struct {
    const char *name;
    test_case_fun test_code;
} test_case;

int test_main(
    int argc,
    char **argv,
    test_setup *setup,
    uint test_count,
    test_case *test_cases
) {
    FILE *stream = stderr;

    // settings
    const char *no_color = getenv("NO_COLOR");
    if (no_color) {
        color_enabled = 0;
    } else if (!isatty(2)) {
        color_enabled = 0;
    }
    const char *assert_trap = getenv("ASSERT_TRAP");
    if (assert_trap) {
        trap_on_assert_fail = 1;
    }
    const char *assert_print_all = getenv("ASSERT_PRINT_ALL");
    if (assert_print_all) {
        print_all_asserts = 1;
    }

    // buffer to back logs
    logs_handle logs_handle = {
        .logs = dynarr_new(4096, uchar, &std_allocator),
    };
    if (!logs_handle.logs) {
        panic();
    }

    // buffer to back assert data
    asserts_handle asserts_handle = {
        .asserts = dynarr_new(1000, test_assert, &std_allocator),
    };
    if (!asserts_handle.asserts) {
        panic();
    }

    // init report
    test_suite_report report = {
        .logs_handle = &logs_handle,
        .asserts_handle = &asserts_handle,
        .test_reports = alloc_new(&std_allocator, test_report, test_count),
        .test_count = test_count,
        .tests_passed = 0,
        .assert_count = 0,
        .asserts_passed = 0
    };
    if (!report.test_reports) {
        panic();
    }
    set_n(report.test_reports, 0, test_count);

    if (setup && setup->before_all) {
        setup->before_all();
    }

    for (size_t i = 0; i < test_count; i += 1) {
        test_case test_case = test_cases[i];
        test_report *tr = &report.test_reports[i];
        tr->name = test_case.name;
        tr->asserts =
            &asserts_handle.asserts[dynarr_len(asserts_handle.asserts)];

        test t = {
            .logs_handle = &logs_handle,
            .asserts_handle = &asserts_handle,
            .assert_count = 0,
            .asserts_passed = 0,
        };

        bool run_test = argc < 2;
        for (int i = 1; i < argc; i++) {
            if (run_test) {
                break;
            }
            if (cstr_len_unsafe(argv[i])) {
                run_test |=
                    cstr_match_wild_ascii_unsafe(test_case.name, argv[i]);
            }
        }

        if (run_test) {
            if (setup && setup->before) {
                setup->before();
            }

            test_case.test_code(&t);

            if (setup && setup->after) {
                setup->after();
            }
        }

        tr->assert_count += t.assert_count;
        tr->asserts_passed += t.asserts_passed;
        report.assert_count += t.assert_count;
        report.asserts_passed += t.asserts_passed;
        if (t.assert_count == t.asserts_passed) {
            report.tests_passed += 1;
        }
    }

    if (setup && setup->after_all) {
        setup->after_all();
    }

    test_suite_report_pretty(&report, stream);

    alloc_free(&std_allocator, report.test_reports);
    dynarr_free(asserts_handle.asserts);
    dynarr_free(logs_handle.logs);

    int fail_count = (int)(report.test_count - report.tests_passed);
    return fail_count;
}

#define setup_tests(setup, tests) \
    int main(int argc, char **argv) { \
        return test_main( \
            argc, argv, (setup), sizeof(tests) / sizeof(*tests), (tests) \
        ); \
    }

////////////////////////
// Assertions
////////////////////////

#define assert_true(t, c, msg) \
    test_report_append((t), !!(c), (msg), sizeof(msg), __FILE__, __LINE__)

#define assert_false(t, c, msg) assert_true(t, !(c), msg)

#define __assert_cmp_uint(t, c, l, r, cmp, msg) \
    test_report_append_formatted_uint( \
        (t), !!(c), (msg), __FILE__, __LINE__, #cmp, (l), (r) \
    )

#define __assert_cmp_sint(t, c, l, r, cmp, msg) \
    test_report_append_formatted_int( \
        (t), !!(c), (msg), __FILE__, __LINE__, #cmp, (l), (r) \
    )

#define assert_eq_uint(t, a, b, msg) \
    __assert_cmp_uint(t, (a) == (b), a, b, ==, msg)
#define assert_ne_uint(t, a, b, msg) \
    __assert_cmp_uint(t, (a) != (b), a, b, !=, msg)
#define assert_lt_uint(t, a, b, msg) \
    __assert_cmp_uint(t, (a) < (b), a, b, <, msg)
#define assert_le_uint(t, a, b, msg) \
    __assert_cmp_uint(t, (a) <= (b), a, b, <=, msg)
#define assert_gt_uint(t, a, b, msg) \
    __assert_cmp_uint(t, (a) > (b), a, b, >, msg)
#define assert_ge_uint(t, a, b, msg) \
    __assert_cmp_uint(t, (a) >= (b), a, b, >=, msg)

#define assert_eq_sint(t, a, b, msg) \
    __assert_cmp_sint(t, (a) == (b), a, b, ==, msg)
#define assert_ne_sint(t, a, b, msg) \
    __assert_cmp_sint(t, (a) != (b), a, b, !=, msg)
#define assert_lt_sint(t, a, b, msg) \
    __assert_cmp_sint(t, (a) < (b), a, b, <, msg)
#define assert_le_sint(t, a, b, msg) \
    __assert_cmp_sint(t, (a) <= (b), a, b, <=, msg)
#define assert_gt_sint(t, a, b, msg) \
    __assert_cmp_sint(t, (a) > (b), a, b, >, msg)
#define assert_ge_sint(t, a, b, msg) \
    __assert_cmp_sint(t, (a) >= (b), a, b, >=, msg)

#define assert_eq_float(t, l, r, eps, msg) \
    test_report_append_formatted_float( \
        (t), abs((l) - (r)) < (eps), (msg), __FILE__, __LINE__, "==", (l), (r) \
    )

#define assert_eq_bytes(t, l, r, capacity, msg) \
    assert_true(t, bytes_eq((l), (r), (capacity)), msg)

#define assert_ne_bytes(t, l, r, capacity, msg) \
    assert_false(t, bytes_eq((l), (r), (capacity)), msg)

#define assert_eq_cstr(t, l, r, msg) \
    test_report_append_formatted_cstr( \
        (t), \
        cstr_eq_unsafe((l), (r)), \
        (msg), \
        __FILE__, \
        __LINE__, \
        "==", \
        (l), \
        (r) \
    )

#define assert_ne_cstr(t, l, r, msg) \
    test_report_append_formatted_cstr( \
        (t), \
        !cstr_eq_unsafe((l), (r)), \
        (msg), \
        __FILE__, \
        __LINE__, \
        "!=", \
        (l), \
        (r) \
    )

#endif // JP_TESTR_H
