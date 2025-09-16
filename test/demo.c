#include "test.h"

int success() {
    assert_true(1, "jee");
}

int fail() {
    assert_true(0, "nah");
}

run_tests(NULL, success, fail);
