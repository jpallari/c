#include "mt.h"
#include "std.h"
#include "testr.h"
#include <pthread.h>

#define nums_per_item 128UL
#define item_count (nums_per_item * 10000UL)
#define expected_sum (item_count * (item_count + 1) / 2)
#define ringbuf_buffer_size (512UL * 1028UL)

struct producer_ctx {
    ringbuf_spsc *rbuf;
};

struct consumer_ctx {
    ringbuf_spsc *rbuf;
    ullong *sum;
    ullong *count;
};

void *produce_nums(void *ctx_) {
    struct producer_ctx *ctx = ctx_;
    ringbuf_spsc *rbuf = ctx->rbuf;

    ullong buffer[nums_per_item];
    slice s = slice_arr(buffer);

    for (ullong i = 1; i <= item_count; i += nums_per_item) {
        for (ullong j = 0; j < nums_per_item; j += 1) { buffer[j] = i + j; }
        while (!ringbuf_spsc_push(rbuf, s)) {}
    }

    // send termination signal
    buffer[0] = 0;
    s.len = sizeof(ullong);
    while (!ringbuf_spsc_push(rbuf, s)) {}

    return NULL;
}

void *consume_nums(void *ctx_) {
    struct consumer_ctx *ctx = ctx_;
    ringbuf_spsc *rbuf = ctx->rbuf;
    ullong sum = 0;
    ullong count = 0;

    ullong buffer[nums_per_item];

    while (1) {
        bool ok = ringbuf_spsc_pop(rbuf, (uchar *)&buffer, sizeof(buffer));
        if (ok) {
            if (buffer[0] == 0) {
                // termination signal received
                break;
            }

            count += nums_per_item;
            for (uint j = 0; j < nums_per_item; j += 1) { sum += buffer[j]; }
        }
    }

    *ctx->sum = sum;
    *ctx->count = count;

    return NULL;
}

void test_ringbuf_spsc_concurrent(test *t) {
    allocation a = alloc_new(&mmap_allocator, uchar, ringbuf_buffer_size);
    ringbuf_spsc rbuf;
    ringbuf_spsc_init(&rbuf, slice_new(a.ptr, a.len), sizeof(ullong) * nums_per_item);

    ullong sum = 0;
    ullong count = 0;

    struct producer_ctx p_ctx = {
        .rbuf = &rbuf,
    };
    struct consumer_ctx c_ctx = {
        .rbuf = &rbuf,
        .sum = &sum,
        .count = &count,
    };
    pthread_t producer_thread;
    pthread_t consumer_thread;

    int thread_err;

    thread_err = pthread_create(&consumer_thread, NULL, consume_nums, &c_ctx);
    assert_false(t, thread_err, "consumer thread creation must succeed");
    thread_err = pthread_create(&producer_thread, NULL, produce_nums, &p_ctx);
    assert_false(t, thread_err, "producer thread creation must succeed");
    thread_err = pthread_join(producer_thread, NULL);
    assert_false(t, thread_err, "producer thread join must succeed");
    thread_err = pthread_join(consumer_thread, NULL);
    assert_false(t, thread_err, "consumer thread join must succeed");

    assert_eq_uint(t, count, item_count, "item count");
    assert_eq_uint(t, sum, expected_sum, "item sum");

    alloc_free(&mmap_allocator, a);
}

void test_ringbuf_spsc_sequential(test *t) {
    allocation a = alloc_new(&mmap_allocator, uchar, ringbuf_buffer_size);
    ringbuf_spsc rbuf;
    ringbuf_spsc_init(&rbuf, slice_new(a.ptr, a.len), sizeof(ullong) * nums_per_item);

    ullong nums[nums_per_item];
    slice s = slice_arr(nums);

    // write until full
    bool ok = 1;
    ullong sum1 = 0;
    for (ullong i = 0; ok; i += 1) {
        ullong partial_sum = 0;
        for (ullong j = 0; j < nums_per_item; j += 1) {
            nums[j] = j + i;
            partial_sum += j + i;
        }
        ok = ringbuf_spsc_push(&rbuf, s);
        if (ok) {
            sum1 += partial_sum;
        }
    }

    // read until empty
    ok = 1;
    bytes_set(nums, 0, sizeof(nums));
    ullong sum2 = 0;
    while (ok) {
        ok = ringbuf_spsc_pop(&rbuf, (uchar *)nums, sizeof(nums));
        if (ok) {
            for (ullong i = 0; i < nums_per_item; i += 1) {
                sum2 += nums[i];
            }
        }
    }

    assert_eq_uint(t, sum1, sum2, "sums");

    alloc_free(&mmap_allocator, a);
}

static test_case tests[] = {
    {"Ring buffer (SPSC) sequential", test_ringbuf_spsc_sequential},
    {"Ring buffer (SPSC) concurrent", test_ringbuf_spsc_concurrent}
};

setup_tests(NULL, tests)
