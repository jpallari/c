#ifndef JP_TEST_H
#define JP_TEST_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define log_fail(msg) \
    fprintf( \
        stderr, "[FAIL] %s:%d (%s): %s\n", __FILE__, __LINE__, __func__, (msg) \
    )

#define assert_false(c, msg) \
    do { \
        if (c) { \
            log_fail(msg); \
            return 0; \
        } \
    } while (0)

#define assert_true(c, msg) assert_false(!(c), msg)

#define assert_eq(a, b, msg) assert_false((a) == (b), msg)

#define assert_ne(a, b, msg) assert_false((a) != (b), msg)

#define assert_lt(a, b, msg) assert_false((a) < (b), msg)

#define assert_le(a, b, msg) assert_false((a) <= (b), msg)

#define assert_gt(a, b, msg) assert_false((a) > (b), msg)

#define assert_ge(a, b, msg) assert_false((a) >= (b), msg)

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
    if (!test_cases) {
        log_fail("No test cases defined");
        return 1;
    }

    int failed_tests = 0;
    test_case test_case;

    if (setup && setup->before_all) {
        setup->before_all();
    }
    for (size_t i = 0; i < test_count; i += 1) {
        test_case = test_cases[i];
        fprintf(stderr, "[TEST] %s\n", test_case.name);
        if (setup && setup->before) {
            setup->before();
        }
        if (!test_case.test_code()) {
            failed_tests += 1;
        }
        if (setup && setup->after) {
            setup->after();
        }
    }
    if (setup && setup->after_all) {
        setup->after_all();
    }

    fprintf(stderr, "Total failed: %d/%ld\n", failed_tests, test_count);
    return failed_tests;
}

#define setup_tests(setup, tests) \
    int main(int argc, char **argv) { \
        return test_main( \
            argc, argv, &(setup), sizeof(tests) / sizeof(*tests), (tests) \
        ); \
    }

#endif
