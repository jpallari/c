#include "../src/jp.h"
#include "test.h"

int bytes_copy() {
    int fails = 0;
    size_t len = 1000;
    int mismatches = 0;

    int *arr1 = jp_new(int, len, &jp_std_allocator);
    assert_true(arr1, "malloc must succeed");
    int *arr2 = jp_new(int, len, &jp_std_allocator);
    assert_true(arr2, "malloc must succeed");

    // fill data
    for (int i = 0; i < len; i += 1) {
        arr1[i] = 100 + i;
        arr2[i] = 200 + i;
    }

    // copy
    jp_bytes_copy(arr1, arr2, sizeof(int) * len);

    // verify no mismatches
    for (int i = 0; i < len; i += 1) {
        if (arr1[i] != arr2[i]) {
            mismatches += 1;
        }
    }
    assert_eq_inc(mismatches, 0, "no mismatches should be found");

    free(arr1);
    free(arr2);
    return fails;
}

int bytes_move_no_overlap() {
    int fails = 0;
    size_t len = 1000;
    int mismatches = 0;

    int *arr1 = jp_new(int, len, &jp_std_allocator);
    assert_true(arr1, "malloc must succeed");
    int *arr2 = jp_new(int, len, &jp_std_allocator);
    assert_true(arr2, "malloc must succeed");

    // fill data
    for (int i = 0; i < len; i += 1) {
        arr1[i] = 100 + i;
        arr2[i] = 200 + i;
    }

    // move
    jp_bytes_move(arr1, arr2, sizeof(int) * len);

    // verify no mismatches
    for (int i = 0; i < len; i += 1) {
        if (arr1[i] != arr2[i]) {
            mismatches += 1;
        }
    }
    assert_eq_inc(mismatches, 0, "no mismatches should be found");

    free(arr1);
    free(arr2);
    return fails;
}

int bytes_move_overlap_left() {
    int fails = 0;
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[2];
    int *right = &arr[4];

    // move
    jp_bytes_move(left, right, sizeof(int) * 4);

    // check moves
    assert_eq_inc(arr[0], 10, "0: remains the same");
    assert_eq_inc(arr[1], 11, "1: remains the same");
    assert_eq_inc(arr[2], 14, "2: moved");
    assert_eq_inc(arr[3], 15, "3: moved");
    assert_eq_inc(arr[4], 16, "4: moved");
    assert_eq_inc(arr[5], 17, "5: moved");
    assert_eq_inc(arr[6], 16, "6: remains the same");
    assert_eq_inc(arr[7], 17, "7: remains the same");

    return fails;
}

int bytes_move_overlap_right() {
    int fails = 0;
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[1];
    int *right = &arr[3];

    // move
    jp_bytes_move(right, left, sizeof(int) * 4);

    // check moves
    assert_eq_inc(arr[0], 10, "0: remains the same");
    assert_eq_inc(arr[1], 11, "1: remains the same");
    assert_eq_inc(arr[2], 12, "2: remains the same");
    assert_eq_inc(arr[3], 11, "3: moved");
    assert_eq_inc(arr[4], 12, "4: moved");
    assert_eq_inc(arr[5], 13, "5: moved");
    assert_eq_inc(arr[6], 14, "6: moved");
    assert_eq_inc(arr[7], 17, "7: remains the same");

    return fails;
}

static test_case tests[] = {
    {"Bytes copy", bytes_copy},
    {"Bytes move no overlap", bytes_move_no_overlap},
    {"Bytes move overlap left", bytes_move_overlap_left},
    {"Bytes move overlap right", bytes_move_overlap_right}
};

setup_tests(NULL, tests)
