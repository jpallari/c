#include "../src/jp.h"
#include "test.h"

int dynarr_push() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_ne(arr, NULL, "array must not be null");

    // check initial values
    assert_eq_inc(
        jp_dynarr_get_capacity(arr), capacity, "capacity must remain the same"
    );
    assert_eq_inc(jp_dynarr_get_count(arr), 0, "count must start at zero");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        value = 100 + i;
        assert_true_inc(
            jp_dynarr_push(arr, &value, 1),
            "push must succeed when there is capacity left"
        );
        assert_eq_inc(
            jp_dynarr_get_capacity(arr),
            capacity,
            "capacity must remain the same"
        );
        assert_eq_inc(jp_dynarr_get_count(arr), i + 1, "count must update");
        assert_eq_inc(arr[i], value, "value must be found from the array");
    }
    assert_eq_inc(
        jp_dynarr_get_capacity(arr),
        jp_dynarr_get_count(arr),
        "capacity and count are the same after last push"
    );

    // attempt to push a value beyond capacity
    value = 200;
    assert_false_inc(
        jp_dynarr_push(arr, &value, 1),
        "push does not succeed when capacity is met"
    );
    assert_eq_inc(
        jp_dynarr_get_capacity(arr), capacity, "capacity must remain the same"
    );
    assert_eq_inc(
        jp_dynarr_get_capacity(arr),
        jp_dynarr_get_count(arr),
        "capacity and count are the same after failed push"
    );

    // verify contents remain the same
    for (int i = 0; i < capacity; i += 1) {
        assert_eq_inc(arr[i], 100 + i, "array contents must remain the same");
    }

    // exit
    jp_dynarr_free(arr);
    return fails;
}

int dynarr_push_grow() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_ne(arr, NULL, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        value = 100 + i;
        jp_dynarr_push(arr, &value, 1);
    }

    // push one more over the capacity
    value = 1000;
    jp_dynarr_push_grow(arr, &value, 1, int);
    assert_eq_inc(jp_dynarr_get_count(arr), 6, "count must increase");
    assert_ge_inc(jp_dynarr_get_capacity(arr), 10, "capacity must increase");
    for (int i = 0; i < capacity; i += 1) {
        assert_eq_inc(arr[i], 100 + i, "filled values must remain the same");
    }
    assert_eq_inc(
        arr[jp_dynarr_get_count(arr) - 1],
        1000,
        "last value must exist in the array"
    );

    // exit
    jp_dynarr_free(arr);
    return fails;
}

int dynarr_clone() {
    int fails = 0;

    // initialize an array
    u64 capacity = 5;
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_ne(arr, NULL, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        int value = 100 + i;
        jp_dynarr_push(arr, &value, 1);
    }

    // clone array
    int *arr_clone = jp_dynarr_clone(arr, capacity * 2, int);
    assert_ne(arr, NULL, "cloned array must not be null");
    assert_eq_inc(
        jp_dynarr_get_capacity(arr_clone),
        capacity * 2,
        "capacity must be doubled"
    );
    assert_eq_inc(
        jp_dynarr_get_count(arr),
        jp_dynarr_get_count(arr_clone),
        "sizes must be same"
    );

    for (int i = 0; i < capacity; i += 1) {
        assert_eq_inc(arr[i], arr_clone[i], "elements must be the same");
    }

    // exit
    jp_dynarr_free(arr);
    jp_dynarr_free(arr_clone);
    return fails;
}

int dynarr_pop() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_true(arr, "array must not be null");

    // prepare array of 2 values
    value = 10;
    assert_true_inc(jp_dynarr_push(arr, &value, 1), "push must succeed");
    value = 20;
    assert_true_inc(jp_dynarr_push(arr, &value, 1), "push must succeed");
    assert_eq_inc(jp_dynarr_get_count(arr), 2, "expect 2 items");

    // 1st pop
    assert_true_inc(jp_dynarr_pop(arr, value), "pop must succeed");
    assert_eq_inc(value, 20, "first pop value must be 20");
    assert_eq_inc(jp_dynarr_get_count(arr), 1, "expect 1 item remaining");

    // 2nd pop
    assert_true_inc(jp_dynarr_pop(arr, value), "pop must succeed");
    assert_eq_inc(value, 10, "second pop value must be 10");
    assert_eq_inc(jp_dynarr_get_count(arr), 0, "expect 0 items remaining");

    // 3rd pop
    assert_false_inc(jp_dynarr_pop(arr, value), "pop must fail");
    assert_eq_inc(value, 10, "previous value must still be present");
    assert_eq_inc(jp_dynarr_get_count(arr), 0, "expect 0 items remaining");

    // exit
    jp_dynarr_free(arr);
    return fails;
}

int dynarr_remove() {
    int fails = 0;

    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_ne(arr, NULL, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        value = 100 + i;
        jp_dynarr_push(arr, &value, 1);
    }

    // remove element from out of bounds
    assert_false_inc(jp_dynarr_remove(arr, 5), "OOB removal must fail");
    assert_eq_inc(
        jp_dynarr_get_count(arr), 5, "array count must remain the same"
    );
    for (int i = 0; i < capacity; i += 1) {
        assert_eq_inc(arr[i], 100 + i, "array must remain the same");
    }

    // remove element from the middle
    assert_true_inc(
        jp_dynarr_remove(arr, 2), "removing from the middle must succeed"
    );
    assert_eq_inc(
        jp_dynarr_get_count(arr), 4, "array must have one element less"
    );
    assert_eq_inc(arr[0], 100, "0: remain the same");
    assert_eq_inc(arr[1], 101, "1: remain the same");
    assert_eq_inc(arr[2], 103, "2: shift left");
    assert_eq_inc(arr[3], 104, "3: shift left");

    jp_dynarr_free(arr);
    return fails;
}

static test_case tests[] = {
    {"Dynamic array push", dynarr_push},
    {"Dynamic array push grow", dynarr_push_grow},
    {"Dynamic array clone", dynarr_clone},
    {"Dynamic array pop", dynarr_pop},
    {"Dynamic array remove", dynarr_remove}
};

setup_tests(NULL, tests)
