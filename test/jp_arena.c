#include "jp.h"
#include "testr.h"

void test_arena(test *t) {
    int buffer[10] = {0};
    size_t buf1_count = 5, buf2_count = 4;
    jp_arena arena = jp_arena_new((u8 *)buffer, sizeof(buffer));
    jp_allocator alloc = jp_arena_allocator_new(&arena);
    size_t buffer_size = sizeof(buffer);
    assert_eq_uint(t, arena.size, buffer_size, "arena size");

    // allocate 1st arena buffer
    int *buf1 = jp_new(int, buf1_count, &alloc);
    size_t buf1_size = sizeof(int) * buf1_count;
    assert_eq_uint(t, arena.used, buf1_size, "arena used after first alloc");

    // allocate 2nd arena buffer
    int *buf2 = jp_new(int, buf2_count, &alloc);
    size_t buf2_size = sizeof(int) * (buf1_count + buf2_count);
    assert_eq_uint(t, arena.used, buf2_size, "arena used after second alloc");

    // fill arena buffers
    for (size_t i = 0; i < buf1_count; i += 1) { buf1[i] = 100 + (int)i; }
    for (size_t i = 0; i < buf2_count; i += 1) { buf2[i] = 200 + (int)i; }

    // check that original buffer was modified as expected
    int expected_buffer[] = {100, 101, 102, 103, 104, 200, 201, 202, 203, 0};
    assert_eq_bytes(
        t,
        (u8 *)buffer,
        (u8 *)expected_buffer,
        sizeof(buffer),
        "buffer must contain stored values"
    );

    // try to overallocate
    int *buf3 = jp_new(int, 3, &alloc);
    assert_false(t, buf3, "overallocation must fail");
}

static test_case tests[] = {{"Arena", test_arena}};

setup_tests(NULL, tests)
