#include "std.h"
#include "testr.h"

void test_dynarr_push(test *t) {
    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = dynarr_new(capacity, int, &std_allocator);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    }

    // check initial values
    assert_eq_uint(
        t, dynarr_capacity(arr), capacity, "capacity must remain the same"
    );
    assert_eq_uint(t, dynarr_len(arr), 0L, "length must start at zero");

    // fill the array
    for (u64 i = 0; i < capacity; i += 1) {
        value = 100 + (int)i;
        assert_true(
            t,
            dynarr_push(arr, &value, 1),
            "push must succeed when there is capacity left"
        );
        assert_eq_uint(
            t, dynarr_capacity(arr), capacity, "capacity must remain the same"
        );
        assert_eq_uint(t, dynarr_len(arr), (u64)(i + 1), "length must update");
        assert_eq_sint(t, arr[i], value, "value must be found from the array");
    }
    assert_eq_uint(
        t,
        dynarr_capacity(arr),
        dynarr_len(arr),
        "capacity and count are the same after last push"
    );

    // attempt to push a value beyond capacity
    value = 200;
    assert_false(
        t,
        dynarr_push(arr, &value, 1),
        "push does not succeed when capacity is met"
    );
    assert_eq_uint(
        t, dynarr_capacity(arr), capacity, "capacity must remain the same"
    );
    assert_eq_uint(
        t,
        dynarr_capacity(arr),
        dynarr_len(arr),
        "capacity and count are the same after failed push"
    );

    // verify contents remain the same
    for (u64 i = 0; i < capacity; i += 1) {
        assert_eq_sint(
            t, arr[i], 100 + (int)i, "array contents must remain the same"
        );
    }

    // exit
    dynarr_free(arr);
}

void test_dynarr_push_grow(test *t) {
    u64 capacity = 5;
    int value = 0;
    int arr_after_fill[] = {100, 101, 102, 103, 104};

    // initialize an array
    int *arr = dynarr_new(capacity, int, &std_allocator);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    };

    // fill the array
    for (u64 i = 0; i < capacity; i += 1) {
        value = 100 + (int)i;
        dynarr_push(arr, &value, 1);
    }

    // push one more over the capacity
    value = 1000;
    arr = dynarr_push_grow(arr, &value, 1, int);
    assert_true(t, arr, "allocation must succeed");
    assert_eq_uint(t, dynarr_len(arr), 6L, "count must increase");
    assert_ge_uint(t, dynarr_capacity(arr), 10L, "capacity must increase");
    assert_eq_bytes(
        t,
        arr,
        arr_after_fill,
        sizeof(arr_after_fill),
        "array must remain the same"
    );
    assert_eq_sint(
        t, arr[dynarr_len(arr) - 1], 1000, "last value must exist in the array"
    );

    // exit
    dynarr_free(arr);
}

void test_dynarr_clone(test *t) {
    u64 capacity = 5;
    u64 capacity_increase = 3;

    // initialize an array
    int *arr = dynarr_new(capacity, int, &std_allocator);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    };

    // fill the array
    for (u64 i = 0; i < capacity; i += 1) {
        int value = 100 + (int)i;
        dynarr_push(arr, &value, 1);
    }

    // clone array
    int *arr_clone = dynarr_clone(arr, capacity_increase, int);
    if (!assert_true(t, arr, "cloned array must not be null")) {
        return;
    };

    assert_eq_uint(
        t,
        dynarr_capacity(arr_clone),
        capacity + capacity_increase,
        "capacity must be increased"
    );
    assert_eq_uint(
        t, dynarr_len(arr), dynarr_len(arr_clone), "lengths must be same"
    );

    assert_eq_bytes(
        t, arr, arr_clone, sizeof(int) * capacity, "elements must be the same"
    );

    // exit
    dynarr_free(arr);
    dynarr_free(arr_clone);
}

void test_dynarr_pop(test *t) {
    u64 capacity = 5;
    int value = 0;

    // initialize an array
    int *arr = dynarr_new(capacity, int, &std_allocator);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    };

    // prepare array of 2 values
    value = 10;
    assert_true(t, dynarr_push(arr, &value, 1), "push must succeed");
    value = 20;
    assert_true(t, dynarr_push(arr, &value, 1), "push must succeed");
    assert_eq_uint(t, dynarr_len(arr), 2L, "expect 2 items");

    // 1st pop
    assert_true(t, dynarr_pop(arr, value), "pop must succeed");
    assert_eq_sint(t, value, 20, "first pop value must be 20");
    assert_eq_uint(t, dynarr_len(arr), 1L, "expect 1 item remaining");

    // 2nd pop
    assert_true(t, dynarr_pop(arr, value), "pop must succeed");
    assert_eq_sint(t, value, 10, "second pop value must be 10");
    assert_eq_uint(t, dynarr_len(arr), 0L, "expect 0 items remaining");

    // 3rd pop
    assert_false(t, dynarr_pop(arr, value), "pop must fail");
    assert_eq_sint(t, value, 10, "previous value must still be present");
    assert_eq_uint(t, dynarr_len(arr), 0L, "expect 0 items remaining");

    // exit
    dynarr_free(arr);
}

void test_dynarr_remove(test *t) {
    u64 capacity = 5;
    int value = 0;
    int arr_after_fill[] = {100, 101, 102, 103, 104};

    // initialize an array
    int *arr = dynarr_new(capacity, int, &std_allocator);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    };

    // fill the array
    for (u64 i = 0; i < capacity; i += 1) {
        value = 100 + (int)i;
        dynarr_push(arr, &value, 1);
    }

    // remove element from out of bounds
    assert_false(t, dynarr_remove(arr, 5), "OOB removal must fail");
    assert_eq_uint(t, dynarr_len(arr), 5L, "array count must remain the same");
    assert_eq_bytes(
        t,
        arr,
        arr_after_fill,
        sizeof(arr_after_fill),
        "array must remain the same"
    );

    // remove element from the middle
    assert_true(
        t, dynarr_remove(arr, 2), "removing from the middle must succeed"
    );
    assert_eq_uint(t, dynarr_len(arr), 4L, "array must have one element less");
    assert_eq_sint(t, arr[0], 100, "0: remain the same");
    assert_eq_sint(t, arr[1], 101, "1: remain the same");
    assert_eq_sint(t, arr[2], 103, "2: shift left");
    assert_eq_sint(t, arr[3], 104, "3: shift left");

    dynarr_free(arr);
}

void test_dynarr_remove_uo(test *t) {
    u64 capacity = 5;
    int value = 0;
    int arr_after_fill[] = {100, 101, 102, 103, 104};

    // initialize an array
    int *arr = dynarr_new(capacity, int, &std_allocator);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    };

    // fill the array
    for (u64 i = 0; i < capacity; i += 1) {
        value = 100 + (int)i;
        dynarr_push(arr, &value, 1);
    }

    // remove element from out of bounds
    assert_false(t, dynarr_remove_uo(arr, 5), "OOB removal must fail");
    assert_eq_uint(t, dynarr_len(arr), 5L, "array count must remain the same");
    assert_eq_bytes(
        t,
        arr,
        arr_after_fill,
        sizeof(arr_after_fill),
        "array must remain the same"
    );

    // remove element from the middle
    assert_true(
        t, dynarr_remove_uo(arr, 2), "removing from the middle must succeed"
    );
    assert_eq_uint(t, dynarr_len(arr), 4L, "array must have one element less");
    assert_eq_sint(t, arr[0], 100, "0: remain the same");
    assert_eq_sint(t, arr[1], 101, "1: remain the same");
    assert_eq_sint(t, arr[2], 104, "2: swapped");
    assert_eq_sint(t, arr[3], 103, "3: remain the same");

    dynarr_free(arr);
}

void test_dynarr_grow_in_arena(test *t) {
    _Alignas(max_align_t) u8 buffer[100] = {0};
    arena arena = arena_new(buffer, sizeof(buffer));
    allocator alloc = arena_allocator_new(&arena);

    // prepare array
    char *arr = dynarr_new(5, char, &alloc);
    if (!assert_true(t, arr, "array must not be null")) {
        return;
    }
    const char *items = "hello";
    assert_true(t, dynarr_push(arr, items, 5), "push must succeed");

    // grow array
    char *new_arr = dynarr_grow(arr, 10, char);
    assert_eq_uint(
        t, dynarr_capacity(arr), 20L, "existing capacity must double and add 10"
    );
    assert_eq_uint(
        t,
        (uintptr_t)arr,
        (uintptr_t)new_arr,
        "new and old array must be the same"
    );

    // push more stuff to array
    items = " world!";
    assert_true(t, dynarr_push(arr, items, 8), "push must succeed");

    // check backing buffer
    dynarr_header *buf_head = (dynarr_header *)buffer;
    assert_eq_uint(
        t,
        buf_head->capacity,
        20L,
        "array head on backing buffer start: capacity"
    );
    assert_eq_uint(
        t, buf_head->len, 13L, "array head on backing buffer start: length"
    );
    assert_eq_cstr(
        t,
        (char *)buffer + sizeof(dynarr_header),
        "hello world!",
        "backing buffer must have the array contents"
    );
}

static test_case tests[] = {
    {"Dynamic array push", test_dynarr_push},
    {"Dynamic array push grow", test_dynarr_push_grow},
    {"Dynamic array clone", test_dynarr_clone},
    {"Dynamic array pop", test_dynarr_pop},
    {"Dynamic array remove", test_dynarr_remove},
    {"Dynamic array remove unordered", test_dynarr_remove_uo},
    {"Dynamic array grow in an arena", test_dynarr_grow_in_arena}
};

setup_tests(NULL, tests)
