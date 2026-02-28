#include "std.h"
#include "testr.h"

void test_round_up_multiple(test *t) {
    assert_eq_uint(t, round_up_multiple_ullong(5, 5), 5, "Round up 5, 5");
    assert_eq_uint(t, round_up_multiple_ullong(1, 3), 3, "Round up 1, 3");
    assert_eq_uint(t, round_up_multiple_ullong(6, 4), 8, "Round up 6, 4");
    assert_eq_uint(t, round_up_multiple_ullong(14, 6), 18, "Round up 14, 6");
}

static test_case tests[] = {
    {"Round up to multiple", test_round_up_multiple},
};

setup_tests(NULL, tests)
