/**
 * Library of common C multi-threading (MT) utilities
 */
#ifndef JP_MT_H
#define JP_MT_H

#ifndef L1D_CACHE_LINESIZE
#define L1D_CACHE_LINESIZE 64
#endif

#include "std.h"
#include <stdatomic.h>

////////////////////////
// Ring buffer (SPSC)
////////////////////////

typedef struct {
    uchar *buffer;
    size_t item_size;
    size_t max_items;
    alignas(L1D_CACHE_LINESIZE) atomic_size_t read_idx;
    alignas(L1D_CACHE_LINESIZE) size_t cached_read_idx;
    alignas(L1D_CACHE_LINESIZE) atomic_size_t write_idx;
    alignas(L1D_CACHE_LINESIZE) size_t cached_write_idx;
} ringbuf_spsc;

bool ringbuf_spsc_init(ringbuf_spsc *rbuf, slice buffer, size_t item_size);

bool ringbuf_spsc_push(ringbuf_spsc *rbuf, slice s);

bool ringbuf_spsc_pop(ringbuf_spsc *rbuf, uchar *buffer, size_t len);

typedef struct {
    void *item;
    size_t idx;
} ringbuf_spsc_h;

bool ringbuf_spsc_acquire_write(ringbuf_spsc *rbuf, ringbuf_spsc_h *handle);

void ringbuf_spsc_release_write(ringbuf_spsc *rbuf, ringbuf_spsc_h handle);

bool ringbuf_spsc_acquire_read(ringbuf_spsc *rbuf, ringbuf_spsc_h *handle);

void ringbuf_spsc_release_read(ringbuf_spsc *rbuf, ringbuf_spsc_h handle);

#endif
