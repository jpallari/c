#include "std.h"
#include "testr.h"

void test_most_significant_bit(test *t) {
    assert_eq_uint(t, bits_most_significant(1), 0, "MSB: 1");
    assert_eq_uint(t, bits_most_significant(2), 1, "MSB: 2");
    assert_eq_uint(t, bits_most_significant(3), 1, "MSB: 3");
    assert_eq_uint(t, bits_most_significant(4), 2, "MSB: 4");
    assert_eq_uint(t, bits_most_significant(5), 2, "MSB: 5");
    assert_eq_uint(t, bits_most_significant(6), 2, "MSB: 6");
    assert_eq_uint(t, bits_most_significant(7), 2, "MSB: 7");
    assert_eq_uint(t, bits_most_significant(8), 3, "MSB: 8");
    assert_eq_uint(t, bits_most_significant(9), 3, "MSB: 9");
    assert_eq_uint(t, bits_most_significant(10), 3, "MSB: 10");
    assert_eq_uint(t, bits_most_significant(11), 3, "MSB: 11");
    assert_eq_uint(t, bits_most_significant(12), 3, "MSB: 12");
    assert_eq_uint(t, bits_most_significant(13), 3, "MSB: 13");
    assert_eq_uint(t, bits_most_significant(14), 3, "MSB: 14");
    assert_eq_uint(t, bits_most_significant(15), 3, "MSB: 15");
    assert_eq_uint(t, bits_most_significant(16), 4, "MSB: 16");
    assert_eq_uint(t, bits_most_significant(17), 4, "MSB: 17");
    assert_eq_uint(t, bits_most_significant(18), 4, "MSB: 18");
    assert_eq_uint(t, bits_most_significant(19), 4, "MSB: 19");
    assert_eq_uint(t, bits_most_significant(20), 4, "MSB: 20");
    assert_eq_uint(t, bits_most_significant(21), 4, "MSB: 21");
    assert_eq_uint(t, bits_most_significant(22), 4, "MSB: 22");
    assert_eq_uint(t, bits_most_significant(23), 4, "MSB: 23");
    assert_eq_uint(t, bits_most_significant(24), 4, "MSB: 24");
    assert_eq_uint(t, bits_most_significant(25), 4, "MSB: 25");
    assert_eq_uint(t, bits_most_significant(26), 4, "MSB: 26");
    assert_eq_uint(t, bits_most_significant(27), 4, "MSB: 27");
    assert_eq_uint(t, bits_most_significant(28), 4, "MSB: 28");
    assert_eq_uint(t, bits_most_significant(29), 4, "MSB: 29");
    assert_eq_uint(t, bits_most_significant(30), 4, "MSB: 30");
    assert_eq_uint(t, bits_most_significant(31), 4, "MSB: 31");
    assert_eq_uint(t, bits_most_significant(32), 5, "MSB: 32");
}

static test_case tests[] = {
    {"Most significant bit", test_most_significant_bit},
};

setup_tests(NULL, tests)
