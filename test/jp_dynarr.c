#include "jp.h"
#include "test.h"

int test_dynarr_push() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_true_bail(arr, "array must not be null");

    // check initial values
    assert_eq(
        jp_dynarr_capacity(arr),
        capacity,
        "%ld",
        "capacity must remain the same"
    );
    assert_eq(
        jp_dynarr_len(arr), 0L, "%ld", "length must start at zero"
    );

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        value = 100 + i;
        assert_true(
            jp_dynarr_push(arr, &value, 1),
            "push must succeed when there is capacity left"
        );
        assert_eq(
            jp_dynarr_capacity(arr),
            capacity,
            "%ld",
            "capacity must remain the same"
        );
        assert_eq(
            jp_dynarr_len(arr), (u64)(i + 1), "%ld", "length must update"
        );
        assert_eq(
            arr[i], value, "%d", "value must be found from the array"
        );
    }
    assert_eq(
        jp_dynarr_capacity(arr),
        jp_dynarr_len(arr),
        "%ld",
        "capacity and count are the same after last push"
    );

    // attempt to push a value beyond capacity
    value = 200;
    assert_false(
        jp_dynarr_push(arr, &value, 1),
        "push does not succeed when capacity is met"
    );
    assert_eq(
        jp_dynarr_capacity(arr),
        capacity,
        "%ld",
        "capacity must remain the same"
    );
    assert_eq(
        jp_dynarr_capacity(arr),
        jp_dynarr_len(arr),
        "%ld",
        "capacity and count are the same after failed push"
    );

    // verify contents remain the same
    for (int i = 0; i < capacity; i += 1) {
        assert_eq(
            arr[i], 100 + i, "%d", "array contents must remain the same"
        );
    }

    // exit
    jp_dynarr_free(arr);
    return fails;
}

int test_dynarr_push_grow() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_true_bail(arr, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        value = 100 + i;
        jp_dynarr_push(arr, &value, 1);
    }

    // push one more over the capacity
    value = 1000;
    arr = jp_dynarr_push_grow(arr, &value, 1, int);
    assert_true(arr, "allocation must succeed");
    assert_eq(jp_dynarr_len(arr), 6L, "%ld", "count must increase");
    assert_ge(
        jp_dynarr_capacity(arr), 10L, "%ld", "capacity must increase"
    );
    for (int i = 0; i < capacity; i += 1) {
        assert_eq(
            arr[i], 100 + i, "%d", "filled values must remain the same"
        );
    }
    assert_eq(
        arr[jp_dynarr_len(arr) - 1],
        1000,
        "%d",
        "last value must exist in the array"
    );

    // exit
    jp_dynarr_free(arr);
    return fails;
}

int test_dynarr_clone() {
    int fails = 0;

    // initialize an array
    u64 capacity = 5;
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_true_bail(arr, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        int value = 100 + i;
        jp_dynarr_push(arr, &value, 1);
    }

    // clone array
    int *arr_clone = jp_dynarr_clone(arr, capacity * 2);
    assert_true_bail(arr, "cloned array must not be null");
    assert_eq(
        jp_dynarr_capacity(arr_clone),
        capacity * 2,
        "%ld",
        "capacity must be doubled"
    );
    assert_eq(
        jp_dynarr_len(arr),
        jp_dynarr_len(arr_clone),
        "%ld",
        "lengths must be same"
    );

    for (int i = 0; i < capacity; i += 1) {
        assert_eq(arr[i], arr_clone[i], "%d", "elements must be the same");
    }

    // exit
    jp_dynarr_free(arr);
    jp_dynarr_free(arr_clone);
    return fails;
}

int test_dynarr_pop() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_true_bail(arr, "array must not be null");

    // prepare array of 2 values
    value = 10;
    assert_true(jp_dynarr_push(arr, &value, 1), "push must succeed");
    value = 20;
    assert_true(jp_dynarr_push(arr, &value, 1), "push must succeed");
    assert_eq(jp_dynarr_len(arr), 2L, "%ld", "expect 2 items");

    // 1st pop
    assert_true(jp_dynarr_pop(arr, value), "pop must succeed");
    assert_eq(value, 20, "%d", "first pop value must be 20");
    assert_eq(
        jp_dynarr_len(arr), 1L, "%ld", "expect 1 item remaining"
    );

    // 2nd pop
    assert_true(jp_dynarr_pop(arr, value), "pop must succeed");
    assert_eq(value, 10, "%d", "second pop value must be 10");
    assert_eq(
        jp_dynarr_len(arr), 0L, "%ld", "expect 0 items remaining"
    );

    // 3rd pop
    assert_false(jp_dynarr_pop(arr, value), "pop must fail");
    assert_eq(value, 10, "%d", "previous value must still be present");
    assert_eq(
        jp_dynarr_len(arr), 0L, "%ld", "expect 0 items remaining"
    );

    // exit
    jp_dynarr_free(arr);
    return fails;
}

int test_dynarr_remove() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_true_bail(arr, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        value = 100 + i;
        jp_dynarr_push(arr, &value, 1);
    }

    // remove element from out of bounds
    assert_false(jp_dynarr_remove(arr, 5), "OOB removal must fail");
    assert_eq(
        jp_dynarr_len(arr), 5L, "%ld", "array count must remain the same"
    );
    for (int i = 0; i < capacity; i += 1) {
        assert_eq(arr[i], 100 + i, "%d", "array must remain the same");
    }

    // remove element from the middle
    assert_true(
        jp_dynarr_remove(arr, 2), "removing from the middle must succeed"
    );
    assert_eq(
        jp_dynarr_len(arr), 4L, "%ld", "array must have one element less"
    );
    assert_eq(arr[0], 100, "%d", "0: remain the same");
    assert_eq(arr[1], 101, "%d", "1: remain the same");
    assert_eq(arr[2], 103, "%d", "2: shift left");
    assert_eq(arr[3], 104, "%d", "3: shift left");

    jp_dynarr_free(arr);
    return fails;
}

static test_case tests[] = {
    {"Dynamic array push", test_dynarr_push},
    {"Dynamic array push grow", test_dynarr_push_grow},
    {"Dynamic array clone", test_dynarr_clone},
    {"Dynamic array pop", test_dynarr_pop},
    {"Dynamic array remove", test_dynarr_remove}
};

setup_tests(NULL, tests)
