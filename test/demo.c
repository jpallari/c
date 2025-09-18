#include "test.h"

int success() {
    assert_true(1, "jee");
    return 1;
}

int fail() {
    assert_true(0, "nah");
    return 1;
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

static test_setup setup = {before_all, before, after, after_all};

static test_case tests[] = {{"Success", success}, {"Fail", fail}};

setup_tests(setup, tests)
