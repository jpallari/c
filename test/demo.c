#include "test.h"

int success() {
    assert_true(1, "jee");
    return 0;
}

int fail() {
    assert_true(0, "nah");
    return 0;
}

int fail_2() {
    int fails = 0;
    assert_false_inc(1, "nah2");
    assert_false_inc(1, "nah3");
    return fails;
}

void before(void) {
    printf("Executing before\n");
}

void before_all(void) {
    printf("Executing before_all\n");
}

void after(void) {
    printf("Executing after\n");
}

void after_all(void) {
    printf("Executing after_all\n");
}

static test_case tests[] = {{"Success", success}, {"Fail", fail}, {"Fail 2", fail_2}};

setup_tests(NULL, tests)
