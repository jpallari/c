#ifndef JP_TESTR_H
#define JP_TESTR_H

#include "std.h"

////////////////////////
// Panic
////////////////////////

/**
 * Quit the program immediately
 */
#define panic() exit(10)

////////////////////////
// Reporting
////////////////////////

typedef struct {
    size_t logs_offset;
    size_t logs_len;
    const char *file;
    int line;
    bool passed;
} test_assert;

typedef struct {
    test_assert *asserts; // dynamic array
} asserts_handle;

typedef struct {
    bytebuf *logs;
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
    bytebuf *logs;
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
);

bool test_report_append_formatted_cstr(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const char *left,
    const char *right
);

bool test_report_append_formatted_cstrl(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const char *left,
    const char *right,
    size_t len
);

bool test_report_append_formatted_hex(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const slice_const left,
    const slice_const right
);

bool test_report_append_formatted_float(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const double left,
    const double right
);

bool test_report_append_formatted_int(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    int line,
    const char *cmp,
    const llong left,
    const llong right
);

bool test_report_append_formatted_uint(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    int line,
    const char *cmp,
    const ullong left,
    const ullong right
);

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
);

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
    test_report_append((t), !!(c), (msg), lengthof(msg), __FILE__, __LINE__)

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

#define assert_eq_cstrl(t, l, r, len, msg) \
    test_report_append_formatted_cstrl( \
        (t), \
        cstr_eq((l), (r), (len)), \
        (msg), \
        __FILE__, \
        __LINE__, \
        "==", \
        (l), \
        (r), \
        (len) \
    )

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

#define assert_eq_bytes(t, l, r, len, msg) \
    test_report_append_formatted_hex( \
        (t), \
        bytes_eq((l), (r), (len)), \
        (msg), \
        __FILE__, \
        __LINE__, \
        "==", \
        slice_const_new((l), (len)), \
        slice_const_new((r), (len)) \
    )

#define assert_ne_bytes(t, l, r, len, msg) \
    test_report_append_formatted_hex( \
        (t), \
        !bytes_eq((l), (r), (len)), \
        (msg), \
        __FILE__, \
        __LINE__, \
        "!=", \
        (l), \
        (r) \
    )

#endif // JP_TESTR_H
