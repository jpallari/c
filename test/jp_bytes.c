#include "jp.h"
#include "test.h"

int test_bytes_copy() {
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
    assert_eq_inc(mismatches, 0, "%d", "no mismatches should be found");

    jp_free(arr1, &jp_std_allocator);
    jp_free(arr2, &jp_std_allocator);
    return fails;
}

int test_bytes_move_no_overlap() {
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
    assert_eq_inc(mismatches, 0, "%d", "no mismatches should be found");

    jp_free(arr1, &jp_std_allocator);
    jp_free(arr2, &jp_std_allocator);
    return fails;
}

int test_bytes_move_overlap_left() {
    int fails = 0;
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[2];
    int *right = &arr[4];

    // move
    jp_bytes_move(left, right, sizeof(int) * 4);

    // check moves
    assert_eq_inc(arr[0], 10, "%d", "0: remains the same");
    assert_eq_inc(arr[1], 11, "%d", "1: remains the same");
    assert_eq_inc(arr[2], 14, "%d", "2: moved");
    assert_eq_inc(arr[3], 15, "%d", "3: moved");
    assert_eq_inc(arr[4], 16, "%d", "4: moved");
    assert_eq_inc(arr[5], 17, "%d", "5: moved");
    assert_eq_inc(arr[6], 16, "%d", "6: remains the same");
    assert_eq_inc(arr[7], 17, "%d", "7: remains the same");

    return fails;
}

int test_bytes_move_overlap_right() {
    int fails = 0;
    int arr[] = {10, 11, 12, 13, 14, 15, 16, 17};
    int *left = &arr[1];
    int *right = &arr[3];

    // move
    jp_bytes_move(right, left, sizeof(int) * 4);

    // check moves
    assert_eq_inc(arr[0], 10, "%d", "0: remains the same");
    assert_eq_inc(arr[1], 11, "%d", "1: remains the same");
    assert_eq_inc(arr[2], 12, "%d", "2: remains the same");
    assert_eq_inc(arr[3], 11, "%d", "3: moved");
    assert_eq_inc(arr[4], 12, "%d", "4: moved");
    assert_eq_inc(arr[5], 13, "%d", "5: moved");
    assert_eq_inc(arr[6], 14, "%d", "6: moved");
    assert_eq_inc(arr[7], 17, "%d", "7: remains the same");

    return fails;
}

static test_case tests[] = {
    {"Bytes copy", test_bytes_copy},
    {"Bytes move no overlap", test_bytes_move_no_overlap},
    {"Bytes move overlap left", test_bytes_move_overlap_left},
    {"Bytes move overlap right", test_bytes_move_overlap_right}
};

setup_tests(NULL, tests)
