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
    slice buffer;
    size_t item_size;
    alignas(L1D_CACHE_LINESIZE) atomic_size_t read_idx;
    alignas(L1D_CACHE_LINESIZE) size_t cached_read_idx;
    alignas(L1D_CACHE_LINESIZE) atomic_size_t write_idx;
    alignas(L1D_CACHE_LINESIZE) size_t cached_write_idx;
} ringbuf_spsc;

void ringbuf_spsc_init(ringbuf_spsc *rbuf, slice buffer, size_t item_size);

bool ringbuf_spsc_push(ringbuf_spsc *rbuf, slice s);

bool ringbuf_spsc_pop(ringbuf_spsc *rbuf, uchar *buffer, size_t len);

#endif
