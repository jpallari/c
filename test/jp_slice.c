#include "jp.h"
#include "test.h"

int slice_span() {
    int fails = 0;

    u8 arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    jp_slice s1 = jp_slice_span(&arr[2], &arr[6]);
    jp_slice s2 = jp_slice_span(&arr[6], &arr[2]);

    assert_eq_inc(s1.size, 4L, "%ld", "slice 1 length 4");
    assert_eq_inc(s2.size, 4L, "%ld", "slice 2 length 4");
    assert_eq_inc(s1.buffer[0], 12, "%d", "slice 1 index 0");
    assert_eq_inc(s1.buffer[1], 13, "%d", "slice 1 index 1");
    assert_eq_inc(s1.buffer[2], 14, "%d", "slice 1 index 2");
    assert_eq_inc(s1.buffer[3], 15, "%d", "slice 1 index 3");
    assert_eq_inc(s2.buffer[0], 12, "%d", "slice 1 index 0");
    assert_eq_inc(s2.buffer[1], 13, "%d", "slice 1 index 1");
    assert_eq_inc(s2.buffer[2], 14, "%d", "slice 1 index 2");
    assert_eq_inc(s2.buffer[3], 15, "%d", "slice 1 index 3");

    return fails;
}

int slice_equal() {
    int fails = 0;

    u8 arr1[] = {0, 0, 0, 11, 12, 13, 0, 0};
    u8 arr2[] = {0, 11, 12, 13, 0, 0, 0, 0};
    jp_slice s1_1 = jp_slice_span(&arr1[3], &arr1[6]);
    jp_slice s1_2 = jp_slice_span(&arr1[1], &arr1[4]);
    jp_slice s1_3 = jp_slice_span(&arr1[3], &arr1[7]);
    jp_slice s2_1 = jp_slice_span(&arr2[1], &arr2[4]);

    assert_true_inc(jp_slice_equal(s1_1, s1_1), "equals with itself");
    assert_false_inc(jp_slice_equal(s1_1, s1_2), "not equals with shifted");
    assert_false_inc(
        jp_slice_equal(s1_1, s1_3), "not equals with different length"
    );
    assert_true_inc(jp_slice_equal(s1_1, s2_1), "equals with similar");

    return fails;
}

int slice_from() {
    int fails = 0;

    int arr[] = {100, 200, 300, 400};
    jp_slice s1 = jp_slice_from("hello world!");
    jp_slice s2 = jp_slice_from(arr);

    assert_eq_inc(s1.size, 13L, "%ld", "s1 size must be size of string");
    assert_eq_inc(
        s2.size, sizeof(int) * 4, "%ld", "s2 size must be size of array"
    );
    assert_eq_cstr(
        (char *)s1.buffer, "hello world!", "s1 contents should be the same"
    );

    return fails;
}

static test_case tests[] = {
    {"Slice span", slice_span},
    {"Slice equal", slice_equal},
    {"Slice from", slice_from}
};

setup_tests(NULL, tests)
