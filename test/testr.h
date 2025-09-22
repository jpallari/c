#ifndef JP_TESTR_H
#define JP_TESTR_H

#include "jp.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

////////////////////////
// Panic
////////////////////////

#define panic __builtin_unreachable

////////////////////////
// Config
////////////////////////

static int color_enabled = 1;
static int trap_on_assert_fail = 0;

////////////////////////
// Reporting
////////////////////////

typedef struct {
    u64 logs_offset;
    const char *file;
    const char *func;
    s32 line;
    b32 passed;
} test_assert;

typedef struct {
    char *name;
    u8 **logs_handle; // dynamic array
    test_assert *asserts; // dynamic array
    u32 assert_count;
    u32 asserts_passed;
} test;

typedef struct {
    test *test_reports;
    u64 test_count;
    u64 tests_passed;
    u32 assert_count;
    u32 asserts_passed;
} test_suite_report;

void test_report_append(
    test *t,
    b32 passed,
    jp_slice log_message,
    const char *file,
    int line,
    const char *func
) {
    assert(t && "test report must not be null");
    assert(t->logs_handle && "logs pointer must not be null");
    assert(*t->logs_handle && "logs storage must not be null");
    u64 logs_offset = jp_dynarr_len(*t->logs_handle);

    test_assert assert_report = {
        .logs_offset = logs_offset,
        .passed = passed,
        .file = file,
        .line = line,
        .func = func,
    };
    test_assert *next_ar =
        jp_dynarr_push_grow(t->asserts, &assert_report, 1, test_assert);
    if (next_ar) {
        t->asserts = next_ar;
    } else {
        panic();
    }

    u8 *next_logs =
        jp_dynarr_push_grow(*t->logs_handle, log_message.buffer, log_message.len, u8);
    if (next_logs) {
        *t->logs_handle = next_logs;
    } else {
        panic();
    }

    t->assert_count += 1;
    if (passed) {
        t->asserts_passed += 1;
    }
}

void test_suite_report_pretty(test_suite_report *report, FILE *stream) {
    assert(report && "test suite report must not be null");

    if (!report->test_count) {
        fprintf(stream, "No tests executed");
        return;
    }

    for (int i = 0; i < report->test_count; i += 1) {
        test tr = report->test_reports[i];
        const char *prefix =
            tr.assert_count > tr.asserts_passed ? "FAIL" : "OK";
        fprintf(
            stream,
            "[ %s ] %s (%d/%d passed)\n",
            prefix,
            tr.name,
            tr.asserts_passed,
            tr.assert_count
        );
        for (int j = 0; j < jp_dynarr_len(tr.asserts); j += 1) {
            test_assert ar = tr.asserts[j];
            if (!ar.passed) {
                const char *prefix = ar.passed ? "OK" : "FAIL";
                fprintf(
                    stream,
                    "    [ %s ] %s:%d (%s): %s\n",
                    prefix,
                    ar.file,
                    ar.line,
                    ar.func,
                    (char *)(&((*tr.logs_handle)[ar.logs_offset]))
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
    FILE *stream = stderr;

    // settings
    const char *no_color = getenv("NO_COLOR");
    if (no_color) {
        color_enabled = 0;
    }
    const char *assert_trap = getenv("ASSERT_TRAP");
    if (assert_trap) {
        trap_on_assert_fail = 1;
    }

    u8 *logs = jp_dynarr_new(4096, u8, &jp_std_allocator);
    if (!logs) {
        panic();
    }

    test_case test_case;
    test_suite_report report = {
        .test_reports = jp_new(test, test_count, &jp_std_allocator),
        .test_count = test_count,
        .tests_passed = 0,
        .assert_count = 0,
        .asserts_passed = 0
    };
    if (!report.test_reports) {
        panic();
    }
    jp_zero_n(report.test_reports, test_count);

    if (setup && setup->before_all) {
        setup->before_all();
    }

    for (size_t i = 0; i < test_count; i += 1) {
        test_case = test_cases[i];
        test *t = &report.test_reports[i];
        t->name = test_case.name;
        t->logs_handle = &logs;
        t->asserts = jp_dynarr_new(10, test_assert, &jp_std_allocator);
        if (!t->asserts) {
            panic();
        }

        if (setup && setup->before) {
            setup->before();
        }

        test_case.test_code(t);
        report.assert_count += t->assert_count;
        report.asserts_passed += t->asserts_passed;
        if (t->assert_count == t->asserts_passed) {
            report.tests_passed += 1;
        }

        if (setup && setup->after) {
            setup->after();
        }
    }

    if (setup && setup->after_all) {
        setup->after_all();
    }

    test_suite_report_pretty(&report, stream);

    for (size_t i = 0; i < test_count; i += 1) {
        jp_dynarr_free(report.test_reports[i].asserts);
    }
    jp_dynarr_free(logs);
    free(report.test_reports);

    return report.test_count - report.tests_passed;
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

#define breakpoint __asm__("int3; nop")

#define assert_true(t, c, msg) \
    do { \
        b32 __result = !!(c); \
        test_report_append( \
            (t), __result, jp_slice_from((msg)), __FILE__, __LINE__, __func__ \
        ); \
        if (!__result && trap_on_assert_fail) { \
            breakpoint; \
        } \
    } while (0)

#define assert_false(t, c, msg) assert_true(t, !(c), msg)

#define __assert_cmp_log_msg(buf, l, r, cmp, f, msg) \
    snprintf((buf), sizeof(buf), (f " " #cmp " " f " // %s"), (l), (r), (msg))

#define __assert_cmp(t, c, l, r, cmp, f, msg) \
    do { \
        char __assert_log_msg[1024] = {0}; \
        __assert_cmp_log_msg(__assert_log_msg, l, r, cmp, f, msg); \
        assert_true(t, c, __assert_log_msg); \
    } while (0)

#define assert_eq(t, a, b, f, msg) __assert_cmp(t, (a) == (b), a, b, ==, f, msg)
#define assert_ne(t, a, b, f, msg) __assert_cmp(t, (a) != (b), a, b, !=, f, msg)
#define assert_lt(t, a, b, f, msg) __assert_cmp(t, (a) < (b), a, b, <, f, msg)
#define assert_le(t, a, b, f, msg) __assert_cmp(t, (a) <= (b), a, b, <=, f, msg)
#define assert_gt(t, a, b, f, msg) __assert_cmp(t, (a) > (b), a, b, >, f, msg)
#define assert_ge(t, a, b, f, msg) __assert_cmp(t, (a) >= (b), a, b, >=, f, msg)

#define assert_eq_bytes(t, l, r, capacity, msg) \
    assert_true(t, jp_bytes_eq((l), (r), (capacity)), msg)

#define assert_ne_bytes(t, l, r, capacity, msg) \
    assert_false(t, jp_bytes_eq((l), (r), (capacity)), msg)

#define assert_eq_cstr(t, l, r, msg) \
    __assert_cmp(t, jp_cstr_eq_unsafe((l), (r)), l, r, ==, "%s", msg)

#define assert_ne_cstr(t, l, r, msg) \
    __assert_cmp(t, !jp_cstr_eq_unsafe((l), (r)), l, r, !=, "%s", msg)

#endif // JP_TESTR_H
