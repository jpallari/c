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
// Breakpoint
////////////////////////

// Use a builtin for breakpoints if possible
#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define breakpoint() __builtin_debugtrap()
#elif __has_builtin(__debugbreak)
#define breakpoint() __debugbreak()
#endif
#endif

// ASM based breakpoint
#if !defined(breakpoint)
#if defined(__i386__) || defined(__x86_64__)
static inline void breakpoint(void) {
    __asm__ __volatile__("int3");
}
#elif defined(__aarch64__)
static inline void breakpoint(void) {
    __asm__ __volatile__(".inst 0xd4200000");
}
#elif defined(__arm__)
static inline void breakpoint(void) {
    __asm__ __volatile__(".inst 0xe7f001f0");
}
#else
// Breakpoint using signals
#if !defined(breakpoint)
#include <signal.h>
#if defined(SIGTRAP)
#define breakpoint() raise(SIGTRAP)
#else
#define breakpoint() raise(SIGABRT)
#endif
#endif
#endif
#endif

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
    s32 line;
    b32 passed;
} test_assert;

typedef struct {
    u8 *logs; // dynamic array
} logs_handle;

typedef struct {
    test_assert *asserts; // dynamic array
} asserts_handle;

typedef struct {
    logs_handle *logs_handle;
    asserts_handle *asserts_handle;
    u32 assert_count;
    u32 asserts_passed;
} test;

typedef struct {
    const char *name;
    test_assert *asserts;
    u32 assert_count;
    u32 asserts_passed;
} test_report;

typedef struct {
    logs_handle *logs_handle;
    asserts_handle *asserts_handle;
    test_report *test_reports;
    u64 test_count;
    u64 tests_passed;
    u32 assert_count;
    u32 asserts_passed;
} test_suite_report;

b32 test_report_append(
    test *t,
    b32 passed,
    const char *log_message,
    size_t log_message_size,
    const char *file,
    int line,
    const char *format,
    const void *left,
    const void *right
) {
    assert(t && "test report must not be null");
    assert(t->logs_handle && "logs handle must not be null");
    assert(t->logs_handle->logs && "logs storage must not be null");

    char buffer[1024] = {0};
    u64 logs_offset = jp_dynarr_len(t->logs_handle->logs);

    test_assert assert_report = {
        .logs_offset = logs_offset,
        .passed = passed,
        .file = file,
        .line = line,
    };
    test_assert *next_ar = jp_dynarr_push_grow(
        t->asserts_handle->asserts, &assert_report, 1, test_assert
    );
    if (next_ar) {
        t->asserts_handle->asserts = next_ar;
    } else {
        panic();
    }

    const char *log = log_message;
    if (format && left && right) {
        snprintf(buffer, sizeof(buffer), format, left, right, log_message);
        log = buffer;
    }

    u8 *next_logs =
        jp_dynarr_push_grow(t->logs_handle->logs, log, log_message_size, u8);
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

void test_suite_report_pretty(test_suite_report *report, FILE *stream) {
    assert(report && "test suite report must not be null");

    const char *color_ok = "\x1B[32m";
    const char *color_fail = "\x1B[31m";
    const char *color_reset = "\x1B[0m";
    const char *color_info = "\x1B[1;30m";
    if (!color_enabled) {
        color_ok = "";
        color_fail = "";
        color_reset = "";
        color_info = "";
    }

    if (!report->test_count) {
        fprintf(stream, "No tests executed");
        return;
    }

    const u8 *logs = report->logs_handle->logs;

    for (u64 i = 0; i < report->test_count; i += 1) {
        test_report tr = report->test_reports[i];
        const char *prefix =
            tr.assert_count > tr.asserts_passed ? "FAIL" : " OK ";
        const char *color =
            tr.assert_count > tr.asserts_passed ? color_fail : color_ok;
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
        for (u32 j = 0; j < tr.assert_count; j += 1) {
            test_assert ar = tr.asserts[j];
            if (!ar.passed) {
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

    // buffer to back logs
    logs_handle logs_handle = {
        .logs = jp_dynarr_new(4096, u8, &jp_std_allocator),
    };
    if (!logs_handle.logs) {
        panic();
    }

    // buffer to back assert data
    asserts_handle asserts_handle = {
        .asserts = jp_dynarr_new(1000, test_assert, &jp_std_allocator),
    };
    if (!asserts_handle.asserts) {
        panic();
    }

    // init report
    test_suite_report report = {
        .logs_handle = &logs_handle,
        .asserts_handle = &asserts_handle,
        .test_reports = jp_new(test_report, test_count, &jp_std_allocator),
        .test_count = test_count,
        .tests_passed = 0,
        .assert_count = 0,
        .asserts_passed = 0
    };
    if (!report.test_reports) {
        panic();
    }
    jp_set_n(report.test_reports, 0, test_count);

    if (setup && setup->before_all) {
        setup->before_all();
    }

    for (size_t i = 0; i < test_count; i += 1) {
        test_case test_case = test_cases[i];
        test t = {
            .logs_handle = &logs_handle,
            .asserts_handle = &asserts_handle,
            .assert_count = 0,
            .asserts_passed = 0,
        };
        test_report *tr = &report.test_reports[i];
        tr->name = test_case.name;
        tr->asserts =
            &asserts_handle.asserts[jp_dynarr_len(asserts_handle.asserts)];

        if (setup && setup->before) {
            setup->before();
        }

        test_case.test_code(&t);

        if (setup && setup->after) {
            setup->after();
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

    free(report.test_reports);
    jp_dynarr_free(asserts_handle.asserts);
    jp_dynarr_free(logs_handle.logs);

    u64 fail_count = report.test_count - report.tests_passed;
    return (int)fail_count;
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
    test_report_append( \
        (t), \
        !!(c), \
        (msg), \
        sizeof(msg), \
        __FILE__, \
        __LINE__, \
        NULL, \
        NULL, \
        NULL \
    )

#define assert_false(t, c, msg) assert_true(t, !(c), msg)

#define __assert_cmp(t, c, l, r, cmp, format, msg) \
    test_report_append( \
        (t), \
        !!(c), \
        (msg), \
        sizeof(msg), \
        __FILE__, \
        __LINE__, \
        (format " " #cmp " " format " // %s"), \
        &(l), \
        &(r) \
    )

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
