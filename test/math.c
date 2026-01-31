#include "std.h"
#include "testr.h"

void test_round_up_multiple(test *t) {
    assert_eq_uint(t, round_up_multiple_ullong(5, 5), 5, "Round up 5, 5");
    assert_eq_uint(t, round_up_multiple_ullong(1, 3), 3, "Round up 1, 3");
    assert_eq_uint(t, round_up_multiple_ullong(6, 4), 8, "Round up 6, 4");
    assert_eq_uint(t, round_up_multiple_ullong(14, 6), 18, "Round up 14, 6");
}

void test_most_significant_bit(test *t) {
    assert_eq_uint(t, most_significant_bit(1), 0, "MSB: 1");
    assert_eq_uint(t, most_significant_bit(2), 1, "MSB: 2");
    assert_eq_uint(t, most_significant_bit(3), 1, "MSB: 3");
    assert_eq_uint(t, most_significant_bit(4), 2, "MSB: 4");
    assert_eq_uint(t, most_significant_bit(5), 2, "MSB: 5");
    assert_eq_uint(t, most_significant_bit(6), 2, "MSB: 6");
    assert_eq_uint(t, most_significant_bit(7), 2, "MSB: 7");
    assert_eq_uint(t, most_significant_bit(8), 3, "MSB: 8");
    assert_eq_uint(t, most_significant_bit(9), 3, "MSB: 9");
    assert_eq_uint(t, most_significant_bit(10), 3, "MSB: 10");
    assert_eq_uint(t, most_significant_bit(11), 3, "MSB: 11");
    assert_eq_uint(t, most_significant_bit(12), 3, "MSB: 12");
    assert_eq_uint(t, most_significant_bit(13), 3, "MSB: 13");
    assert_eq_uint(t, most_significant_bit(14), 3, "MSB: 14");
    assert_eq_uint(t, most_significant_bit(15), 3, "MSB: 15");
    assert_eq_uint(t, most_significant_bit(16), 4, "MSB: 16");
    assert_eq_uint(t, most_significant_bit(17), 4, "MSB: 17");
    assert_eq_uint(t, most_significant_bit(18), 4, "MSB: 18");
    assert_eq_uint(t, most_significant_bit(19), 4, "MSB: 19");
    assert_eq_uint(t, most_significant_bit(20), 4, "MSB: 20");
    assert_eq_uint(t, most_significant_bit(21), 4, "MSB: 21");
    assert_eq_uint(t, most_significant_bit(22), 4, "MSB: 22");
    assert_eq_uint(t, most_significant_bit(23), 4, "MSB: 23");
    assert_eq_uint(t, most_significant_bit(24), 4, "MSB: 24");
    assert_eq_uint(t, most_significant_bit(25), 4, "MSB: 25");
    assert_eq_uint(t, most_significant_bit(26), 4, "MSB: 26");
    assert_eq_uint(t, most_significant_bit(27), 4, "MSB: 27");
    assert_eq_uint(t, most_significant_bit(28), 4, "MSB: 28");
    assert_eq_uint(t, most_significant_bit(29), 4, "MSB: 29");
    assert_eq_uint(t, most_significant_bit(30), 4, "MSB: 30");
    assert_eq_uint(t, most_significant_bit(31), 4, "MSB: 31");
    assert_eq_uint(t, most_significant_bit(32), 5, "MSB: 32");
}

static test_case tests[] = {
    {"Round up to multiple", test_round_up_multiple},
    {"Most significant bit", test_most_significant_bit},
};

setup_tests(NULL, tests)
