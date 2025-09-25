#include "jp.h"
#include "testr.h"

void test_slice_span(test *t) {
    u8 arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    jp_slice s1 = jp_slice_span(&arr[2], &arr[6]);
    jp_slice s2 = jp_slice_span(&arr[6], &arr[2]);

    assert_eq(t, s1.len, 4L, "%ld", "slice 1 length 4");
    assert_eq(t, s2.len, 4L, "%ld", "slice 2 length 4");
    assert_eq(t, s1.buffer[0], 12, "%d", "slice 1 index 0");
    assert_eq(t, s1.buffer[1], 13, "%d", "slice 1 index 1");
    assert_eq(t, s1.buffer[2], 14, "%d", "slice 1 index 2");
    assert_eq(t, s1.buffer[3], 15, "%d", "slice 1 index 3");
    assert_eq(t, s2.buffer[0], 12, "%d", "slice 2 index 0");
    assert_eq(t, s2.buffer[1], 13, "%d", "slice 2 index 1");
    assert_eq(t, s2.buffer[2], 14, "%d", "slice 2 index 2");
    assert_eq(t, s2.buffer[3], 15, "%d", "slice 2 index 3");
}

void test_slice_equal(test *t) {
    u8 arr1[] = {0, 0, 0, 11, 12, 13, 0, 0};
    u8 arr2[] = {0, 11, 12, 13, 0, 0, 0, 0};
    jp_slice s1_1 = jp_slice_span(&arr1[3], &arr1[6]);
    jp_slice s1_2 = jp_slice_span(&arr1[1], &arr1[4]);
    jp_slice s1_3 = jp_slice_span(&arr1[3], &arr1[7]);
    jp_slice s2_1 = jp_slice_span(&arr2[1], &arr2[4]);

    assert_true(t, jp_slice_eq(s1_1, s1_1), "equals with itself");
    assert_false(t, jp_slice_eq(s1_1, s1_2), "not equals with shifted");
    assert_false(
        t, jp_slice_eq(s1_1, s1_3), "not equals with different length"
    );
    assert_true(t, jp_slice_eq(s1_1, s2_1), "equals with similar");
}

void test_slice_from(test *t) {
    char str[] = "hello world!";
    int arr[] = {100, 200, 300, 400};
    jp_slice s1 = jp_slice_from(str);
    jp_slice s2 = jp_slice_from(arr);

    assert_eq(t, s1.len, 13L, "%ld", "s1 len must be size of string");
    assert_eq(
        t, s2.len, sizeof(int) * 4, "%ld", "s2 len must be size of array"
    );
    assert_eq_cstr(
        t, (char *)s1.buffer, "hello world!", "s1 contents should be the same"
    );
    assert_eq_bytes(
        t, s2.buffer, (u8 *)arr, s2.len, "s2 contents should be the same"
    );
}

void test_slice_copy_larger(test *t) {
    u8 arr1[] = {10, 11, 12, 13, 14, 15, 16, 17};
    u8 arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    u8 expected_arr1[] = {10, 11, 21, 22, 23, 15, 16, 17};
    u8 expected_arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    jp_slice s1 = jp_slice_span(&arr1[2], &arr1[6]);
    jp_slice s2 = jp_slice_span(&arr2[1], &arr2[4]);

    jp_slice_copy(s1, s2);

    assert_eq_bytes(t, arr1, expected_arr1, sizeof(arr1), "arr1 should change");
    assert_eq_bytes(
        t, s1.buffer, &expected_arr1[2], s1.len, "s1 should change"
    );
    assert_eq_bytes(
        t, arr2, expected_arr2, sizeof(arr2), "arr2 should not change"
    );
    assert_eq_bytes(
        t, s2.buffer, &expected_arr2[1], s2.len, "s2 should not change"
    );
}

void test_slice_copy_smaller(test *t) {
    u8 arr1[] = {10, 11, 12, 13, 14, 15, 16, 17};
    u8 arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    u8 expected_arr1[] = {10, 11, 21, 22, 14, 15, 16, 17};
    u8 expected_arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    jp_slice s1 = jp_slice_span(&arr1[2], &arr1[4]);
    jp_slice s2 = jp_slice_span(&arr2[1], &arr2[6]);

    jp_slice_copy(s1, s2);

    assert_eq_bytes(t, arr1, expected_arr1, sizeof(arr1), "arr1 should change");
    assert_eq_bytes(
        t, s1.buffer, &expected_arr1[2], s1.len, "s1 should change"
    );
    assert_eq_bytes(
        t, arr2, expected_arr2, sizeof(arr2), "arr2 should not change"
    );
    assert_eq_bytes(
        t, s2.buffer, &expected_arr2[1], s2.len, "s2 should not change"
    );
}

void test_slice_move_overlapping(test *t) {
    u8 arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    u8 expected_arr[] = {10, 11, 13, 14, 14, 15, 16, 17};
    jp_slice s1 = jp_slice_span(&arr[2], &arr[4]);
    jp_slice s2 = jp_slice_span(&arr[3], &arr[6]);

    jp_slice_move(s1, s2);

    assert_eq_bytes(t, arr, expected_arr, sizeof(arr), "arr should change");
    assert_eq_bytes(t, s1.buffer, &expected_arr[2], s1.len, "s1 should change");
}

void test_slice_from_cstr_unsafe(test *t) {
    char str[] = "hello world!";
    jp_slice slice = jp_slice_from_cstr_unsafe(str);

    assert_eq(t, slice.len, 13L, "%ld", "slice len must match string length");
    assert_eq(
        t,
        (uintptr_t)slice.buffer,
        (uintptr_t)str,
        "%lu",
        "slice and str pointers must match"
    );
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
