#include "mt.h"
#include "std.h"
#include "testr.h"
#include <pthread.h>

#define nums_per_item 128UL
#define item_count (nums_per_item * 100000UL)
#define expected_sum (item_count * (item_count + 1) / 2)
#define ringbuf_buffer_size (1 << 20)

struct producer_ctx {
    ringbuf_spsc *rbuf;
};

struct consumer_ctx {
    ringbuf_spsc *rbuf;
    ullong *sum;
    ullong *count;
};

void *produce_nums_acquire(void *ctx_) {
    struct producer_ctx *ctx = ctx_;
    ringbuf_spsc *rbuf = ctx->rbuf;

    for (ullong i = 1; i <= item_count; i += nums_per_item) {
        ringbuf_spsc_h h;
        while (!ringbuf_spsc_acquire_write(rbuf, &h));
        ullong *buf = h.item;
        for (ullong j = 0; j < nums_per_item; j += 1) { buf[j] = i + j; }
        ringbuf_spsc_release_write(rbuf, h);
    }

    // send termination signal
    ullong buffer[1] = {0};
    slice s = slice_arr(buffer);
    s.len = sizeof(ullong);
    while (!ringbuf_spsc_push(rbuf, s)) {}

    return NULL;
}

void *consume_nums_acquire(void *ctx_) {
    struct consumer_ctx *ctx = ctx_;
    ringbuf_spsc *rbuf = ctx->rbuf;
    ullong sum = 0;
    ullong count = 0;

    ringbuf_spsc_h h;

    while (1) {
        bool ok = ringbuf_spsc_acquire_read(rbuf, &h);
        if (!ok) {
            continue;
        }
        ullong *buffer = h.item;

        if (buffer[0] == 0) {
            // termination signal received
            break;
        }

        count += nums_per_item;
        for (uint j = 0; j < nums_per_item; j += 1) { sum += buffer[j]; }
        ringbuf_spsc_release_read(rbuf, h);
    }

    *ctx->sum = sum;
    *ctx->count = count;

    return NULL;
}

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
        if (!ok) {
            continue;
        }
        if (buffer[0] == 0) {
            // termination signal received
            break;
        }

        count += nums_per_item;
        for (uint j = 0; j < nums_per_item; j += 1) { sum += buffer[j]; }
    }

    *ctx->sum = sum;
    *ctx->count = count;

    return NULL;
}

void test_ringbuf_spsc_concurrent_with_mode(test *t, int mode) {
    allocation a = alloc_new(&mmap_allocator, uchar, ringbuf_buffer_size);
    ringbuf_spsc rbuf;
    assert_true(
        t,
        ringbuf_spsc_init(
            &rbuf, slice_new(a.ptr, a.len), sizeof(ullong) * nums_per_item
        ),
        "init must succeed"
    );

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

    // Set up consumer and producer functions based on mode
    void *(*consumer_f)(void *);
    void *(*producer_f)(void *);
    if (mode) {
        consumer_f = consume_nums_acquire;
        producer_f = produce_nums_acquire;
    } else {
        consumer_f = consume_nums;
        producer_f = produce_nums;
    }

    int thread_err;
    thread_err = pthread_create(&consumer_thread, NULL, consumer_f, &c_ctx);
    assert_false(t, thread_err, "consumer thread creation must succeed");
    thread_err = pthread_create(&producer_thread, NULL, producer_f, &p_ctx);
    assert_false(t, thread_err, "producer thread creation must succeed");
    thread_err = pthread_join(producer_thread, NULL);
    assert_false(t, thread_err, "producer thread join must succeed");
    thread_err = pthread_join(consumer_thread, NULL);
    assert_false(t, thread_err, "consumer thread join must succeed");

    assert_eq_uint(t, count, item_count, "item count");
    assert_eq_uint(t, sum, expected_sum, "item sum");

    alloc_free(&mmap_allocator, a);
}

void test_ringbuf_spsc_concurrent(test *t) {
    test_ringbuf_spsc_concurrent_with_mode(t, 0);
}

void test_ringbuf_spsc_concurrent_acquire(test *t) {
    test_ringbuf_spsc_concurrent_with_mode(t, 1);
}

void test_ringbuf_spsc_sequential(test *t) {
    allocation a = alloc_new(&mmap_allocator, uchar, ringbuf_buffer_size);
    ringbuf_spsc rbuf;
    assert_true(
        t,
        ringbuf_spsc_init(
            &rbuf, slice_new(a.ptr, a.len), sizeof(ullong) * nums_per_item
        ),
        "init must succeed"
    );

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
            for (ullong i = 0; i < nums_per_item; i += 1) { sum2 += nums[i]; }
        }
    }

    assert_eq_uint(t, sum1, sum2, "sums");

    alloc_free(&mmap_allocator, a);
}

static test_case tests[] = {
    {"Ring buffer (SPSC) sequential", test_ringbuf_spsc_sequential},
    {"Ring buffer (SPSC) concurrent", test_ringbuf_spsc_concurrent},
    {"Ring buffer (SPSC) concurrent w/ acquire", test_ringbuf_spsc_concurrent_acquire}
};

setup_tests(NULL, tests)
