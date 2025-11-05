
////////////////////////
// Ring buffer
////////////////////////

#include "std.h"
typedef struct {
    u8 *buf;
    u64 buf_len;
    u64 read_idx;
    u64 write_idx;
    u64 garbage_idx;
} ringbuf;

void ringbuf_init(ringbuf *rbuf, u8 *buf, u64 buf_len);

u8 *ringbuf_acquire(ringbuf *rbuf, u64 len);

slice ringbuf_read(ringbuf *rbuf, u64 max_len);

////////////////////////
// Ring buffer
////////////////////////

void ringbuf_init(ringbuf *rbuf, u8 *buf, u64 buf_len) {
    assert(rbuf && "rbuf must not be null");
    assert(buf && "buf must not be null");
    assert(buf_len > 0 && "buf length must be greater than zero");

    bytes_set(rbuf, 0, sizeof(*rbuf));
    rbuf->buf = buf;
    rbuf->buf_len = buf_len;
    rbuf->read_idx = 0;
    rbuf->write_idx = 0;
    rbuf->garbage_idx = 0;
}

u8 *ringbuf_acquire(ringbuf *rbuf, u64 len) {
    assert(rbuf && "rbuf must not be null");
    assert(len > 0 && "length must be greater than zero");
    assert(len < rbuf->buf_len && "cannot acquire more than buffer size");

    if (len >= rbuf->buf_len) {
        // cannot aqcuire more than buffer has capacity
        return NULL;
    }

    u64 w, max_len;
    {
        if (rbuf->write_idx) {
            u64 r = rbuf->read_idx % rbuf->buf_len;
            w = rbuf->write_idx % rbuf->buf_len;
            max_len = w > r ? w - r : r - w;
        } else {
            w = 0;
            max_len = len;
        }
    }

    if (max_len < len) {
        // cannot acquire past the read head
        return NULL;
    }

    u64 max_idx = rbuf->write_idx >= rbuf->buf_len
        ? (rbuf->write_idx + rbuf->buf_len) / rbuf->buf_len * rbuf->buf_len
        : rbuf->buf_len;
    if (len + rbuf->write_idx <= max_idx) {
        // no need to wrap around
        rbuf->write_idx += len;
        return rbuf->buf + w;
    }
    if (max_len <= max_idx - rbuf->write_idx + len) {
        // cannot acquire past the read head
        return NULL;
    }

    // wrap around
    rbuf->garbage_idx = rbuf->write_idx;
    rbuf->write_idx = max_idx;
    return rbuf->buf;
}

slice ringbuf_read(ringbuf *rbuf, u64 max_len) {
    assert(rbuf && "rbuf must not be null");
    assert(max_len > 0 && "length must be greater than zero");
    slice s = {0};

    if (rbuf->write_idx == 0) {
        return s;
    }

    u64 r_end_idx = rbuf->garbage_idx
        ? min(rbuf->garbage_idx, rbuf->write_idx - 1)
        : rbuf->write_idx - 1;

    u64 r = rbuf->read_idx % rbuf->buf_len;
    u64 r_end = r_end_idx % rbuf->buf_len;

    u64 available_len = (r_end > r ? r_end - r : rbuf->buf_len - r) + 1;
    u64 read_len = min(available_len, max_len);
    rbuf->read_idx += read_len;

    if (rbuf->garbage_idx) {
        assert(
            rbuf->read_idx > rbuf->garbage_idx && "read cannot be past garbage"
        );
        if (rbuf->read_idx == rbuf->garbage_idx) {
            rbuf->garbage_idx = 0;
            rbuf->read_idx = (rbuf->read_idx + rbuf->buf_len - 1)
                / rbuf->buf_len * rbuf->buf_len;
        }
    }

    s.buffer = rbuf->buf + r;
    s.len = read_len;
    return s;
}
