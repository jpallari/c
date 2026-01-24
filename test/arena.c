#include "std.h"
#include "testr.h"
#include <stddef.h>

void test_arena(test *t) {
    alignas(max_align_t) int buffer[10] = {0};
    size_t buf1_count = 5, buf2_count = 4;
    arena arena = arena_new((uchar *)buffer, sizeof(buffer));
    allocator alloc = arena_allocator_new(&arena);
    size_t buffer_size = sizeof(buffer);
    assert_eq_uint(t, arena.size, buffer_size, "arena size");

    // allocate 1st arena buffer
    slice s_buf1 = alloc_new(&alloc, int, buf1_count);
    int *buf1 = slice_cast(s_buf1, int);
    size_t buf1_size = sizeof(int) * buf1_count;
    assert_eq_uint(t, arena.used, buf1_size, "arena used after first alloc");

    // allocate 2nd arena buffer
    slice s_buf2 = alloc_new(&alloc, int, buf2_count);
    int *buf2 = slice_cast(s_buf2, int);
    size_t buf2_size = sizeof(int) * (buf1_count + buf2_count);
    assert_eq_uint(t, arena.used, buf2_size, "arena used after second alloc");

    // fill arena buffers
    for (size_t i = 0; i < buf1_count; i += 1) { buf1[i] = 100 + (int)i; }
    for (size_t i = 0; i < buf2_count; i += 1) { buf2[i] = 200 + (int)i; }

    // check that original buffer was modified as expected
    int expected_buffer[] = {100, 101, 102, 103, 104, 200, 201, 202, 203, 0};
    assert_eq_bytes(
        t,
        (uchar *)buffer,
        (uchar *)expected_buffer,
        sizeof(buffer),
        "buffer must contain stored values"
    );

    // try to overallocate
    slice s_buf3 = alloc_new(&alloc, int, 3);
    assert_false(t, slice_is_set(s_buf3), "overallocation must fail");
}

static test_case tests[] = {{"Arena", test_arena}};

setup_tests(NULL, tests)
