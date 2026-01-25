#include "std.h"
#include "testr.h"

void test_mmap_allocation(test *t) {
    size_t count = 100;
    allocation s_nums = alloc_new(&mmap_allocator, int, count);
    int *nums = s_nums.ptr;

    assert_true(t, allocation_exists(s_nums), "allocation must succeed");
    assert_ge_uint(
        t,
        s_nums.len,
        sizeof(uint) * 100,
        "allocation size must be at least as large as requested"
    );

    // write some numbers
    for (size_t i = 0; i < count; i += 1) {
        nums[i] = (int)(i % 502) - (502 / 2);
    }

    alloc_free(&mmap_allocator, s_nums);
}

static test_case tests[] = {{"mmap allocation", test_mmap_allocation}};

setup_tests(NULL, tests)
