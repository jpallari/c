#include "../src/jp.h"
#include "test.h"

int dynarr_push() {
    int fails = 0;

    u64 capacity = 5;
    b32 push_result = 0;
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
        push_result = jp_dynarr_push(arr, value);
        assert_true_inc(
            push_result, "push must succeed when there is capacity left"
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
    push_result = jp_dynarr_push(arr, value);
    assert_false_inc(push_result, "push does not succeed when capacity is met");
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

int dynarr_clone() {
    int fails = 0;

    u64 capacity = 5;

    // initialize an array
    int *arr = jp_dynarr_new(capacity, int, &jp_std_allocator);
    assert_ne(arr, NULL, "array must not be null");

    // fill the array
    for (int i = 0; i < capacity; i += 1) {
        int value = 100 + i;
        jp_dynarr_push(arr, value);
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

static test_case tests[] = {
    {"Dynamic array push", dynarr_push}, {"Dynamic array clone", dynarr_clone}
};

setup_tests(NULL, tests)
