#include "std.h"
#include "testr.h"

void test_bytes_copy(test *t) {
    size_t len = 1000;
    int mismatches = 0;

    int *arr1 = alloc_new(&std_allocator, int, len);
    if (!assert_true(t, arr1, "malloc must succeed")) {
        return;
    }

    int *arr2 = alloc_new(&std_allocator, int, len);
    if (!assert_true(t, arr2, "malloc must succeed")) {
        return;
    }

    // fill data
    for (size_t i = 0; i < len; i += 1) {
        arr1[i] = 100 + (int)i;
        arr2[i] = 200 + (int)i;
    }

    // copy
    bytes_copy(arr1, arr2, sizeof(int) * len);

    // verify no mismatches
    for (size_t i = 0; i < len; i += 1) {
        if (arr1[i] != arr2[i]) {
            mismatches += 1;
        }
    }
    int expected_mismatches = 0;
    assert_eq_sint(
        t, mismatches, expected_mismatches, "no mismatches should be found"
    );

    alloc_free(&std_allocator, arr1);
    alloc_free(&std_allocator, arr2);
}

void test_bytes_move_no_overlap(test *t) {
    size_t len = 1000;
    int mismatches = 0;

    int *arr1 = alloc_new(&std_allocator, int, len);
    if (!assert_true(t, arr1, "malloc must succeed")) {
        return;
    };

    int *arr2 = alloc_new(&std_allocator, int, len);
    if (!assert_true(t, arr2, "malloc must succeed")) {
        return;
    };

    // fill data
    for (size_t i = 0; i < len; i += 1) {
        arr1[i] = 100 + (int)i;
        arr2[i] = 200 + (int)i;
    }

    // move
    bytes_move(arr1, arr2, sizeof(int) * len);

    // verify no mismatches
    for (size_t i = 0; i < len; i += 1) {
        if (arr1[i] != arr2[i]) {
            mismatches += 1;
        }
    }
    int expected_mismatches = 0;
    assert_eq_sint(
        t, mismatches, expected_mismatches, "no mismatches should be found"
    );

    alloc_free(&std_allocator, arr1);
    alloc_free(&std_allocator, arr2);
}

void test_bytes_move_overlap_left(test *t) {
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[2];
    int *right = &arr[4];

    // move
    bytes_move(left, right, sizeof(int) * 4);

    // check moves
    assert_eq_sint(t, arr[0], 10, "0: remains the same");
    assert_eq_sint(t, arr[1], 11, "1: remains the same");
    assert_eq_sint(t, arr[2], 14, "2: moved");
    assert_eq_sint(t, arr[3], 15, "3: moved");
    assert_eq_sint(t, arr[4], 16, "4: moved");
    assert_eq_sint(t, arr[5], 17, "5: moved");
    assert_eq_sint(t, arr[6], 16, "6: remains the same");
    assert_eq_sint(t, arr[7], 17, "7: remains the same");
}

void test_bytes_move_overlap_right(test *t) {
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[1];
    int *right = &arr[3];

    // move
    bytes_move(right, left, sizeof(int) * 4);

    // check moves
    assert_eq_sint(t, arr[0], 10, "0: remains the same");
    assert_eq_sint(t, arr[1], 11, "1: remains the same");
    assert_eq_sint(t, arr[2], 12, "2: remains the same");
    assert_eq_sint(t, arr[3], 11, "3: moved");
    assert_eq_sint(t, arr[4], 12, "4: moved");
    assert_eq_sint(t, arr[5], 13, "5: moved");
    assert_eq_sint(t, arr[6], 14, "6: moved");
    assert_eq_sint(t, arr[7], 17, "7: remains the same");
}

void test_bytes_zero(test *t) {
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int expected_arr[] = {0, 0, 0, 0, 0, 0, 0, 0};

    bytes_set(arr, 0, sizeof(arr));

    assert_eq_bytes(t, arr, expected_arr, sizeof(arr), "array must be zeroed");
}

void test_bytes_to_hex(test *t) {
    const char *str = "hello world!";
    const char *expected_str = "68656c6c6f20776f726c6421";
    char dest[25] = {0};

    bytes_to_hex(dest, str, cstr_len_unsafe(str));

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
