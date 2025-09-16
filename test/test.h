#ifndef JP_TEST_H
#define JP_TEST_H

#include <stdarg.h>
#include <stddef.h>

#define log_fail(msg) \
    fprintf( \
        stderr, "[FAIL] %s:%s (%s): %s", __FILE__, __LINE__, __func__, (msg) \
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

typedef struct {
    void (*before_all)();
    void (*before)();
    void (*after)();
    void (*after_all)();
} test_setup;

int test_main(
    int argc, char **argv, test_setup *setup, size_t test_count, ...
) {
    int failed_tests = 0;
    int (*test_func)();
    va_list ap;
    va_start(ap, test_count);
    if (setup && setup->before_all) {
        setup->before_all();
    }
    for (size_t i = 0; i < test_count; i += 1) {
        test_func = va_arg(ap, int (*)());
        if (setup && setup->before) {
            setup->before();
        }
        if (!test_func()) {
            failed_tests += 1;
        }
        if (setup && setup->after) {
            setup->after();
        }
    }
    if (setup && setup->after_all) {
        setup->after_all();
    }
    va_end(ap);
    return failed_tests;
}

#define run_tests(setup, ...) \
    int main(int argc, char **argv) { \
        test_main( \
            argc, \
            argv, \
            sizeof((int[]) {__VA_ARGS__}) / sizeof(int), \
            __VA_ARGS__ \
        ); \
    }

#endif
