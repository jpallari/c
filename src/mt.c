#include "mt.h"
#include "std.h"

//////////////////////////////////////////////
// Ring buffer (SPSC)
//
// Based on https://rigtorp.se/ringbuffer/
/////////////////////////////////////////////

bool ringbuf_spsc_init(ringbuf_spsc *rbuf, slice buffer, size_t item_size) {
    assert(rbuf && "ringbuf must not be null");
    assert(buffer.buffer && "ringbuf buffer must not be null");
    assert(buffer.len > 0 && "ringbuff buffer len must be >0");
    assert(item_size > 0 && "item size must be >0");
    assert(item_size < buffer.len && "item size must be smaller than len");

    if (buffer.buffer == NULL || item_size == 0 || buffer.len == 0) {
        return 0; // funky parameters
    }

    bytes_set(rbuf, 0, sizeof(*rbuf));
    rbuf->buffer = buffer.buffer;
    rbuf->item_size = item_size;
    rbuf->max_items = buffer.len / rbuf->item_size;

    return 1;
}

bool ringbuf_spsc_push(ringbuf_spsc *rbuf, slice s) {
    assert(s.buffer && "buffer must not be null");
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->buffer && "ringbuf buffer must not be null");
    assert(rbuf->item_size > 0 && "item size must be >0");
    assert(rbuf->max_items > 0 && "max items must be >0");
    assert(s.len <= rbuf->item_size && "item cannot be larger than item size");

    if (s.len > rbuf->item_size) {
        // slice buffer is too large
        return 0;
    }

    size_t write_idx =
        atomic_load_explicit(&rbuf->write_idx, memory_order_relaxed);
    size_t next_write_idx = (write_idx + 1) % rbuf->max_items;

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

bool ringbuf_spsc_acquire_write(ringbuf_spsc *rbuf, ringbuf_spsc_h *handle) {
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->buffer && "ringbuf buffer must not be null");
    assert(rbuf->item_size > 0 && "item size must be >0");
    assert(rbuf->max_items > 0 && "max items must be >0");

    size_t write_idx =
        atomic_load_explicit(&rbuf->write_idx, memory_order_relaxed);
    size_t next_write_idx = (write_idx + 1) % rbuf->max_items;

    if (next_write_idx == rbuf->cached_read_idx) {
        rbuf->cached_read_idx =
            atomic_load_explicit(&rbuf->read_idx, memory_order_acquire);
        if (next_write_idx == rbuf->cached_read_idx) {
            // full
            return 0;
        }
    }

    size_t byte_index = rbuf->item_size * write_idx;
    handle->item = rbuf->buffer + byte_index;
    handle->idx = write_idx;
    return 1;
}

void ringbuf_spsc_release_write(ringbuf_spsc *rbuf, ringbuf_spsc_h handle) {
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->max_items > 0 && "max items must be >0");
    assert(handle.item && "item must not be null");

    size_t next_write_idx = (handle.idx + 1) % rbuf->max_items;
    atomic_store_explicit(
        &rbuf->write_idx, next_write_idx, memory_order_release
    );
}

bool ringbuf_spsc_pop(ringbuf_spsc *rbuf, uchar *buffer, size_t len) {
    assert(buffer && "buffer must not be null");
    assert(len && "len must not be null");
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->buffer && "ringbuf buffer must not be null");
    assert(rbuf->item_size > 0 && "item size must be >0");
    assert(rbuf->max_items > 0 && "max items must be >0");

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
    bytes_copy(buffer, rbuf->buffer + byte_index, min(len, rbuf->item_size));
    size_t next_read_idx = (read_idx + 1) % rbuf->max_items;
    atomic_store_explicit(&rbuf->read_idx, next_read_idx, memory_order_release);

    return 1;
}

bool ringbuf_spsc_acquire_read(ringbuf_spsc *rbuf, ringbuf_spsc_h *handle) {
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->buffer && "ringbuf buffer must not be null");
    assert(rbuf->item_size > 0 && "item size must be >0");
    assert(rbuf->max_items > 0 && "max items must be >0");

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
    handle->item = rbuf->buffer + byte_index;
    handle->idx = read_idx;
    return 1;
}

void ringbuf_spsc_release_read(ringbuf_spsc *rbuf, ringbuf_spsc_h handle) {
    assert(rbuf && "ringbuf must not be null");
    assert(rbuf->max_items > 0 && "max items must be >0");
    assert(handle.item && "item must not be null");

    size_t next_read_idx = (handle.idx + 1) % rbuf->max_items;
    atomic_store_explicit(&rbuf->read_idx, next_read_idx, memory_order_release);
}
