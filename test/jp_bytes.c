#include "jp.h"
#include "testr.h"

void test_bytes_copy(test *t) {
    size_t len = 1000;
    int mismatches = 0;

    int *arr1 = jp_new(int, len, &jp_std_allocator);
    if (!assert_true(t, arr1, "malloc must succeed")) {
        return;
    }

    int *arr2 = jp_new(int, len, &jp_std_allocator);
    if (!assert_true(t, arr2, "malloc must succeed")) {
        return;
    }

    // fill data
    for (size_t i = 0; i < len; i += 1) {
        arr1[i] = 100 + (int)i;
        arr2[i] = 200 + (int)i;
    }

    // copy
    jp_bytes_copy(arr1, arr2, sizeof(int) * len);

    // verify no mismatches
    for (size_t i = 0; i < len; i += 1) {
        if (arr1[i] != arr2[i]) {
            mismatches += 1;
        }
    }
    assert_eq(t, mismatches, 0, "%d", "no mismatches should be found");

    jp_free(arr1, &jp_std_allocator);
    jp_free(arr2, &jp_std_allocator);
}

void test_bytes_move_no_overlap(test *t) {
    size_t len = 1000;
    int mismatches = 0;

    int *arr1 = jp_new(int, len, &jp_std_allocator);
    if (!assert_true(t, arr1, "malloc must succeed")) {
        return;
    };

    int *arr2 = jp_new(int, len, &jp_std_allocator);
    if (!assert_true(t, arr2, "malloc must succeed")) {
        return;
    };

    // fill data
    for (size_t i = 0; i < len; i += 1) {
        arr1[i] = 100 + (int)i;
        arr2[i] = 200 + (int)i;
    }

    // move
    jp_bytes_move(arr1, arr2, sizeof(int) * len);

    // verify no mismatches
    for (size_t i = 0; i < len; i += 1) {
        if (arr1[i] != arr2[i]) {
            mismatches += 1;
        }
    }
    assert_eq(t, mismatches, 0, "%d", "no mismatches should be found");

    jp_free(arr1, &jp_std_allocator);
    jp_free(arr2, &jp_std_allocator);
}

void test_bytes_move_overlap_left(test *t) {
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[2];
    int *right = &arr[4];

    // move
    jp_bytes_move(left, right, sizeof(int) * 4);

    // check moves
    assert_eq(t, arr[0], 10, "%d", "0: remains the same");
    assert_eq(t, arr[1], 11, "%d", "1: remains the same");
    assert_eq(t, arr[2], 14, "%d", "2: moved");
    assert_eq(t, arr[3], 15, "%d", "3: moved");
    assert_eq(t, arr[4], 16, "%d", "4: moved");
    assert_eq(t, arr[5], 17, "%d", "5: moved");
    assert_eq(t, arr[6], 16, "%d", "6: remains the same");
    assert_eq(t, arr[7], 17, "%d", "7: remains the same");
}

void test_bytes_move_overlap_right(test *t) {
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[1];
    int *right = &arr[3];

    // move
    jp_bytes_move(right, left, sizeof(int) * 4);

    // check moves
    assert_eq(t, arr[0], 10, "%d", "0: remains the same");
    assert_eq(t, arr[1], 11, "%d", "1: remains the same");
    assert_eq(t, arr[2], 12, "%d", "2: remains the same");
    assert_eq(t, arr[3], 11, "%d", "3: moved");
    assert_eq(t, arr[4], 12, "%d", "4: moved");
    assert_eq(t, arr[5], 13, "%d", "5: moved");
    assert_eq(t, arr[6], 14, "%d", "6: moved");
    assert_eq(t, arr[7], 17, "%d", "7: remains the same");
}

void test_bytes_zero(test *t) {
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int expected_arr[] = {0, 0, 0, 0, 0, 0, 0, 0};

    jp_bytes_set(arr, 0, sizeof(arr));

    assert_eq_bytes(t, arr, expected_arr, sizeof(arr), "array must be zeroed");
}

void test_bytes_to_hex(test *t) {
    const char *str = "hello world!";
    const char *expected_str = "68656c6c6f20776f726c6421";
    char dest[25] = {0};

    jp_bytes_to_hex(dest, str, jp_cstr_len_unsafe(str));

    assert_eq_cstr(
        t, (char *)dest, expected_str, "string converstion to hex string"
    );
}

static test_case tests[] = {
    {"Bytes copy", test_bytes_copy},
    {"Bytes move no overlap", test_bytes_move_no_overlap},
    {"Bytes move overlap left", test_bytes_move_overlap_left},
    {"Bytes move overlap right", test_bytes_move_overlap_right},
    {"Bytes zero", test_bytes_zero},
    {"Bytes to hex", test_bytes_to_hex}
};

setup_tests(NULL, tests)
