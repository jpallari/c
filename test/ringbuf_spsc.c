#include "mt.h"
#include "std.h"
#include "testr.h"
#include <pthread.h>

#define item_count 100000

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

    ullong buffer[10];
    slice s = slice_arr(buffer);

    for (ullong i = 0; i < item_count; i += 10) {
        for (ullong j = 0; j < 10; j += 1) { buffer[j] = i + j; }
        while (!ringbuf_spsc_push(rbuf, s)) {}
    }

    s.len = 0;
    while (!ringbuf_spsc_push(rbuf, s)) {}

    return NULL;
}

void *consume_nums(void *ctx_) {
    struct consumer_ctx *ctx = ctx_;
    ringbuf_spsc *rbuf = ctx->rbuf;
    ullong sum = 0;
    ullong count = 0;

    ullong buffer[10];
    size_t len = 0;

    while (1) {
        bool ok = ringbuf_spsc_pop(rbuf, (uchar *)&buffer, &len);
        if (ok) {
            if (len == 0) {
                break;
            }

            size_t n = len / sizeof(size_t);
            count += n;
            for (uint j = 0; j < n; j += 1) { sum += buffer[j]; }
        }
    }

    *ctx->sum = sum;
    *ctx->count = count;

    return NULL;
}

void test_ringbuf_spsc_concurrent(test *t) {
    uchar buffer[4096] = {0};
    slice s = slice_arr(buffer);
    ringbuf_spsc rbuf;
    ringbuf_spsc_init(&rbuf, s, sizeof(ullong) * 10, alignof(ullong));

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
    assert_eq_uint(t, sum, 4999950000UL, "item sum");
}

void test_ringbuf_spsc_sequential(test *t) {
    uchar buffer[4096] = {0};
    slice s_buffer = slice_arr(buffer);
    ringbuf_spsc rbuf;
    ringbuf_spsc_init(&rbuf, s_buffer, sizeof(ullong) * 10, alignof(ullong));

    ullong nums[10];
    slice s = slice_arr(nums);

    // write until full
    bool ok = 1;
    ullong sum1 = 0;
    for (ullong i = 0; ok; i += 1) {
        ullong partial_sum = 0;
        for (ullong j = 0; j < 10; j += 1) {
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
    size_t len = 0;
    ullong sum2 = 0;
    while (ok) {
        ok = ringbuf_spsc_pop(&rbuf, (uchar *)nums, &len);
        if (ok && len) {
            for (ullong i = 0; i < len/sizeof(size_t); i += 1) {
                sum2 += nums[i];
            }
        }
    }

    assert_eq_uint(t, sum1, sum2, "sums");
}

static test_case tests[] = {
    {"Ring buffer (SPSC) sequential", test_ringbuf_spsc_sequential},
    {"Ring buffer (SPSC) concurrent", test_ringbuf_spsc_concurrent}
};

setup_tests(NULL, tests)
