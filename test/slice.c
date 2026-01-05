#include "std.h"
#include "testr.h"

void test_slice_span(test *t) {
    uchar arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    slice s1 = slice_span(&arr[2], &arr[6]);
    slice s2 = slice_span(&arr[6], &arr[2]);

    assert_eq_uint(t, s1.len, 4L, "slice 1 length 4");
    assert_eq_uint(t, s2.len, 4L, "slice 2 length 4");
    assert_eq_uint(t, s1.buffer[0], 12, "slice 1 index 0");
    assert_eq_uint(t, s1.buffer[1], 13, "slice 1 index 1");
    assert_eq_uint(t, s1.buffer[2], 14, "slice 1 index 2");
    assert_eq_uint(t, s1.buffer[3], 15, "slice 1 index 3");
    assert_eq_uint(t, s2.buffer[0], 12, "slice 2 index 0");
    assert_eq_uint(t, s2.buffer[1], 13, "slice 2 index 1");
    assert_eq_uint(t, s2.buffer[2], 14, "slice 2 index 2");
    assert_eq_uint(t, s2.buffer[3], 15, "slice 2 index 3");
}

void test_slice_equal(test *t) {
    uchar arr1[] = {0, 0, 0, 11, 12, 13, 0, 0};
    uchar arr2[] = {0, 11, 12, 13, 0, 0, 0, 0};
    slice s1_1 = slice_span(&arr1[3], &arr1[6]);
    slice s1_2 = slice_span(&arr1[1], &arr1[4]);
    slice s1_3 = slice_span(&arr1[3], &arr1[7]);
    slice s2_1 = slice_span(&arr2[1], &arr2[4]);

    assert_true(t, slice_eq(s1_1, s1_1), "equals with itself");
    assert_false(t, slice_eq(s1_1, s1_2), "not equals with shifted");
    assert_false(t, slice_eq(s1_1, s1_3), "not equals with different length");
    assert_true(t, slice_eq(s1_1, s2_1), "equals with similar");
}

void test_slice_from_arr(test *t) {
    char str[] = "hello world!";
    int arr[] = {100, 200, 300, 400};
    slice s1 = slice_arr(str);
    slice s2 = slice_arr(arr);

    assert_eq_uint(t, s1.len, 13L, "s1 len must be size of string");
    assert_eq_uint(t, s2.len, sizeof(int) * 4, "s2 len must be size of array");
    assert_eq_cstr(
        t, (char *)s1.buffer, "hello world!", "s1 contents must be the same"
    );
    assert_eq_bytes(
        t, s2.buffer, (uchar *)arr, s2.len, "s2 contents must be the same"
    );
}

void test_slice_from_static_cstr(test *t) {
    slice_const s1 = slice_sstr("hello world!");

    assert_eq_uint(t, s1.len, 12L, "s1 len must be length of string");
    assert_eq_cstr(
        t,
        (const char *)s1.buffer,
        "hello world!",
        "s1 contents must be the same"
    );
}

void test_slice_const_conversion(test *t) {
    char text[] = "hello world!";
    slice s_mut = slice_arr(text);
    slice_const *s_const = (slice_const *)&s_mut;

    assert_eq_uint(t, s_mut.len, s_const->len, "same length");
    assert_eq_cstr(
        t,
        (const char *)s_mut.buffer,
        (const char *)s_const->buffer,
        "same text"
    );
}

void test_slice_copy_larger(test *t) {
    uchar arr1[] = {10, 11, 12, 13, 14, 15, 16, 17};
    uchar arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    uchar expected_arr1[] = {10, 11, 21, 22, 23, 15, 16, 17};
    uchar expected_arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    slice s1 = slice_span(&arr1[2], &arr1[6]);
    slice s2 = slice_span(&arr2[1], &arr2[4]);

    slice_copy(s1, s2);

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
    uchar arr1[] = {10, 11, 12, 13, 14, 15, 16, 17};
    uchar arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    uchar expected_arr1[] = {10, 11, 21, 22, 14, 15, 16, 17};
    uchar expected_arr2[] = {20, 21, 22, 23, 24, 25, 26, 27};
    slice s1 = slice_span(&arr1[2], &arr1[4]);
    slice s2 = slice_span(&arr2[1], &arr2[6]);

    slice_copy(s1, s2);

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
    uchar arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    uchar expected_arr[] = {10, 11, 13, 14, 14, 15, 16, 17};
    slice s1 = slice_span(&arr[2], &arr[4]);
    slice s2 = slice_span(&arr[3], &arr[6]);

    slice_move(s1, s2);

    assert_eq_bytes(t, arr, expected_arr, sizeof(arr), "arr should change");
    assert_eq_bytes(t, s1.buffer, &expected_arr[2], s1.len, "s1 should change");
}

void test_slice_from_cstr_unsafe(test *t) {
    char str[] = "hello world!";
    slice slice = slice_from_cstr_unsafe(str);

    assert_eq_uint(t, slice.len, 12L, "slice len must match string length");
    assert_eq_uint(
        t,
        (uintptr_t)slice.buffer,
        (uintptr_t)str,
        "slice and str pointers must match"
    );
}

static test_case tests[] = {
    {"Slice span", test_slice_span},
    {"Slice equal", test_slice_equal},
    {"Slice from array", test_slice_from_arr},
    {"Slice from static C-string", test_slice_from_static_cstr},
    {"Slice const conversion", test_slice_const_conversion},
    {"Slice copy to larger", test_slice_copy_larger},
    {"Slice copy to smaller", test_slice_copy_smaller},
    {"Slice move overlapping", test_slice_move_overlapping},
    {"Slice from C string (unsafe)", test_slice_from_cstr_unsafe}
};

setup_tests(NULL, tests)
