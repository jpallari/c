#include "jp.h"
#include "test.h"

int test_slice_span() {
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

int test_slice_equal() {
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

int test_slice_from() {
    int fails = 0;

    int arr[] = {100, 200, 300, 400};
    jp_slice s1 = jp_slice_from("hello world!");
    jp_slice s2 = jp_slice_from(arr);

    assert_eq_inc(s1.size, 13L, "%ld", "s1 size must be size of string");
    assert_eq_inc(
        s2.size, sizeof(int) * 4, "%ld", "s2 size must be size of array"
    );
    assert_eq_cstr_inc(
        (char *)s1.buffer, "hello world!", "s1 contents should be the same"
    );
    assert_eq_bytes_inc(
        s2.buffer, (u8 *)arr, s2.size, "s2 contents should be the same"
    );

    return fails;
}

int test_slice_copy_larger() {
    int fails = 0;

    u8 arr1[] = {10, 11, 12, 13, 14, 15, 16, 17};
    u8 arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    u8 expected_arr1[] = {10, 11, 21, 22, 23, 15, 16, 17};
    u8 expected_arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    jp_slice s1 = jp_slice_span(&arr1[2], &arr1[6]);
    jp_slice s2 = jp_slice_span(&arr2[1], &arr2[4]);

    jp_slice_copy(s1, s2);

    assert_eq_bytes_inc(
        arr1, expected_arr1, sizeof(arr1), "arr1 should change"
    );
    assert_eq_bytes_inc(
        s1.buffer, &expected_arr1[2], s1.size, "s1 should change"
    );
    assert_eq_bytes_inc(
        arr2, expected_arr2, sizeof(arr2), "arr2 should not change"
    );
    assert_eq_bytes_inc(
        s2.buffer, &expected_arr2[1], s2.size, "s2 should not change"
    );

    return fails;
}

int test_slice_copy_smaller() {
    int fails = 0;

    u8 arr1[] = {10, 11, 12, 13, 14, 15, 16, 17};
    u8 arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    u8 expected_arr1[] = {10, 11, 21, 22, 14, 15, 16, 17};
    u8 expected_arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    jp_slice s1 = jp_slice_span(&arr1[2], &arr1[4]);
    jp_slice s2 = jp_slice_span(&arr2[1], &arr2[6]);

    jp_slice_copy(s1, s2);

    assert_eq_bytes_inc(
        arr1, expected_arr1, sizeof(arr1), "arr1 should change"
    );
    assert_eq_bytes_inc(
        s1.buffer, &expected_arr1[2], s1.size, "s1 should change"
    );
    assert_eq_bytes_inc(
        arr2, expected_arr2, sizeof(arr2), "arr2 should not change"
    );
    assert_eq_bytes_inc(
        s2.buffer, &expected_arr2[1], s2.size, "s2 should not change"
    );

    return fails;
}

int test_slice_move_overlapping() {
    int fails = 0;

    u8 arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    u8 expected_arr[] = {10, 11, 13, 14, 14, 15, 16, 17};
    jp_slice s1 = jp_slice_span(&arr[2], &arr[4]);
    jp_slice s2 = jp_slice_span(&arr[3], &arr[6]);

    jp_slice_move(s1, s2);

    assert_eq_bytes_inc(arr, expected_arr, sizeof(arr), "arr should change");
    assert_eq_bytes_inc(
        s1.buffer, &expected_arr[2], s1.size, "s1 should change"
    );

    return fails;
}

int test_slice_from_cstr_unsafe() {
    int fails = 0;

    char *str = "hello world!";
    jp_slice slice = jp_slice_from_cstr_unsafe(str);

    assert_eq_inc(
        slice.size, 13L, "%ld", "slice size must match string length"
    );
    assert_eq_inc(
        (uintptr_t)slice.buffer,
        (uintptr_t)str,
        "%lu",
        "slice and str pointers must match"
    );

    return fails;
}

static test_case tests[] = {
    {"Slice span", test_slice_span},
    {"Slice equal", test_slice_equal},
    {"Slice from", test_slice_from},
    {"Slice copy to larger", test_slice_copy_larger},
    {"Slice copy to smaller", test_slice_copy_smaller},
    {"Slice move overlapping", test_slice_move_overlapping},
    {"Slice from C string (unsafe)", test_slice_from_cstr_unsafe}
};

setup_tests(NULL, tests)
