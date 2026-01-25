#include "mt.h"

//////////////////////////////////////////////
// Ring buffer (SPSC)
//
// Based on https://rigtorp.se/ringbuffer/
/////////////////////////////////////////////

static inline size_t ringbuf_spsc_byte_index(ringbuf_spsc *rbuf, size_t item_index) {
    return (rbuf->header_size + rbuf->item_size) * item_index;
}

void ringbuf_spsc_init(
    ringbuf_spsc *rbuf, slice buffer, size_t item_size, size_t alignment
) {
    assert(rbuf && "ringbuf must not be null");
    assert(buffer.buffer && "ringbuf buffer must not be null");
    assert(buffer.len > 0 && "ringbuff buffer len must be >0");
    assert(item_size > 0 && "item size must be >0");
    assert(item_size < buffer.len && "item size must be smaller than len");

    size_t header_size = align_to_nearest(sizeof(size_t), alignment);
    assert(
        header_size + item_size < buffer.len
        && "item and header size must be smaller than buffer len"
    );

    bytes_set(rbuf, 0, sizeof(*rbuf));
    rbuf->buffer = buffer;
    rbuf->header_size = header_size;
    rbuf->item_size = item_size;
    rbuf->max_items = buffer.len / (item_size + header_size);
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

    size_t write_idx =
        atomic_load_explicit(&rbuf->write_idx, memory_order_relaxed);
    size_t next_write_idx = write_idx + 1;
    if (next_write_idx >= rbuf->max_items) {
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
    size_t byte_index = ringbuf_spsc_byte_index(rbuf, write_idx);
    bytes_copy(rbuf->buffer.buffer + byte_index, &s.len, sizeof(s.len));
    bytes_copy(
        rbuf->buffer.buffer + byte_index + rbuf->header_size,
        s.buffer,
        s.len
    );
    atomic_store_explicit(
        &rbuf->write_idx, next_write_idx, memory_order_release
    );
    return 1;
}

bool ringbuf_spsc_pop(ringbuf_spsc *rbuf, uchar *buffer, size_t *len) {
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
    size_t byte_index = ringbuf_spsc_byte_index(rbuf, read_idx);
    bytes_copy(len, rbuf->buffer.buffer + byte_index, sizeof(size_t));
    bytes_copy(
        buffer, rbuf->buffer.buffer + byte_index + rbuf->header_size, *len
    );
    size_t next_read_idx = read_idx + 1;
    if (next_read_idx  >= rbuf->max_items) {
        next_read_idx = 0;
    }
    atomic_store_explicit(&rbuf->read_idx, next_read_idx, memory_order_release);
    return 1;
}
