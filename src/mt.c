#include "mt.h"

//////////////////////////////////////////////
// Ring buffer (SPSC)
//
// Based on https://rigtorp.se/ringbuffer/
/////////////////////////////////////////////

void ringbuf_spsc_init(ringbuf_spsc *rbuf, slice buffer, size_t item_size) {
    assert(rbuf && "ringbuf must not be null");
    assert(buffer.buffer && "ringbuf buffer must not be null");
    assert(buffer.len > 0 && "ringbuff buffer len must be >0");
    assert(item_size > 0 && "item size must be >0");
    assert(item_size < buffer.len && "item size must be smaller than len");

    bytes_set(rbuf, 0, sizeof(*rbuf));
    rbuf->buffer = buffer;
    rbuf->item_size = item_size;
}

bool ringbuf_spsc_push(ringbuf_spsc *rbuf, slice s) {
    assert(s.buffer && "buffer must not be null");
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->buffer.buffer && "ringbuf buffer must not be null");
    assert(rbuf->buffer.len > 0 && "ringbuff buffer len must be >0");
    assert(rbuf->item_size > 0 && "item size must be >0");
    assert(s.len <= rbuf->item_size && "item cannot be larger than item size");

    if (s.len > rbuf->item_size) {
        // slice buffer is too large
        return 0;
    }

    size_t max_items = rbuf->buffer.len / rbuf->item_size;
    size_t write_idx =
        atomic_load_explicit(&rbuf->write_idx, memory_order_relaxed);
    size_t next_write_idx = write_idx + 1;
    if (next_write_idx >= max_items) {
        next_write_idx = 0;
    }
    if (next_write_idx == rbuf->cached_read_idx) {
        rbuf->cached_read_idx =
            atomic_load_explicit(&rbuf->read_idx, memory_order_acquire);
        if (next_write_idx == rbuf->cached_read_idx) {
            // full
            return 0;
        }
    }
    size_t byte_index = rbuf->item_size * write_idx;
    bytes_copy(
        rbuf->buffer + byte_index, s.buffer, min(s.len, rbuf->item_size)
    );
    atomic_store_explicit(
        &rbuf->write_idx, next_write_idx, memory_order_release
    );
    return 1;
}

bool ringbuf_spsc_pop(ringbuf_spsc *rbuf, uchar *buffer, size_t len) {
    assert(buffer && "buffer must not be null");
    assert(len && "len must not be null");
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->buffer.buffer && "ringbuf buffer must not be null");
    assert(rbuf->buffer.len > 0 && "ringbuff buffer len must be >0");
    assert(rbuf->item_size > 0 && "item size must be >0");

    size_t read_idx =
        atomic_load_explicit(&rbuf->read_idx, memory_order_relaxed);
    if (read_idx == rbuf->cached_write_idx) {
        rbuf->cached_write_idx =
            atomic_load_explicit(&rbuf->write_idx, memory_order_acquire);
        if (read_idx == rbuf->cached_write_idx) {
            // empty
            return 0;
        }
    }
    size_t byte_index = rbuf->item_size * read_idx;
    bytes_copy(
        buffer, rbuf->buffer.buffer + byte_index, min(len, rbuf->item_size)
    );
    size_t next_read_idx = read_idx + 1;
    size_t max_items = rbuf->buffer.len / rbuf->item_size;
    if (next_read_idx >= max_items) {
        next_read_idx = 0;
    }
    atomic_store_explicit(&rbuf->read_idx, next_read_idx, memory_order_release);
    return 1;
}
