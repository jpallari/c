#include "std.h"
#include "testr.h"

struct bytesink_ctx_bytebuf {
    bytebuf bbuf;
    size_t chunk_size;
};

static bytesink_result
bytebuf_collect(void *context, const uchar *src, size_t len) {
    bytesink_result res = {0};
    struct bytesink_ctx_bytebuf *ctx = (struct bytesink_ctx_bytebuf *)context;

    if (ctx->chunk_size == 0) {
        bool ok = bytebuf_write(&ctx->bbuf, src, len);
        if (ok) {
            res.len = len;
        } else {
            res.err_code = 1;
        }
        return res;
    }

    // chunked write is used for simulating partially completing writes
    while (res.len < len) {
        size_t chunk_size = min(ctx->chunk_size, len - res.len);
        bool ok = bytebuf_write(&ctx->bbuf, src + res.len, chunk_size);
        if (!ok) {
            res.err_code = 1;
            return res;
        }
        res.len += chunk_size;
    }

    return res;
}

void test_buffered_stream_short_writes(test *t) {
    uchar bytebuf_buf[1024];
    uchar bstream_buf[10];

    struct bytesink_ctx_bytebuf context = {0};
    bytebuf_init_fixed(&context.bbuf, bytebuf_buf, 0, sizeof(bytebuf_buf));
    bufstream bstream = {
        .buffer = bstream_buf,
        .cap = sizeof(bstream_buf),
        .len = 0,
        .sink = {
            .fn = bytebuf_collect,
            .context = &context,
        },
    };

    bufstream_write_result res;
    res = bufstream_write_sstr(&bstream, "hello");
    assert_eq_sint(t, res.err_code, 0, "no error");
    assert_eq_uint(t, res.len, lengthof("hello"), "len = lengthof('hello')");
    assert_eq_uint(t, context.bbuf.len, 0, "sink len = 0");

    res = bufstream_write_sstr(&bstream, " ");
    assert_eq_sint(t, res.err_code, 0, "no error");
    assert_eq_uint(t, res.len, 1, "len = lengthof(' ')");
    assert_eq_uint(t, context.bbuf.len, 0, "sink len = 0");

    res = bufstream_write_sstr(&bstream, "world!");
    assert_eq_sint(t, res.err_code, 0, "no error");
    assert_eq_uint(t, res.len, lengthof("world!"), "len = lengthof('world!')");
    assert_eq_bytes(
        t, context.bbuf.buffer, (const uchar *)"hello worl", 10, "sink content partial"
    );

    bytesink_result sink_res = bufstream_flush(&bstream);
    assert_eq_sint(t, sink_res.err_code, 0, "no error");
    assert_eq_uint(t, sink_res.len, 2, "len = lengthof('d!')");
    assert_eq_bytes(
        t, context.bbuf.buffer, (const uchar *)"hello world!", 12, "sink content full"
    );
}

void test_buffered_stream_long_writes(test *t) {
    uchar bytebuf_buf[1024];
    uchar bstream_buf[10];

    struct bytesink_ctx_bytebuf context = {0};
    bytebuf_init_fixed(&context.bbuf, bytebuf_buf, 0, sizeof(bytebuf_buf));
    bufstream bstream = {
        .buffer = bstream_buf,
        .cap = sizeof(bstream_buf),
        .len = 0,
        .sink = {
            .fn = bytebuf_collect,
            .context = &context,
        },
    };

    bufstream_write_sstr(&bstream, "Tolstoy");
    bufstream_write_result res =
        bufstream_write_sstr(&bstream, " book Anna Karenina");
    assert_eq_sint(t, res.err_code, 0, "no error");
    assert_eq_uint(t, res.len, 19, "length of last write");
    assert_eq_bytes(
        t,
        context.bbuf.buffer,
        (const uchar *)"Tolstoy book Anna Karenina",
        26,
        "sink content full"
    );
    assert_eq_uint(t, bstream.len, 0, "buffer empty");
}

void test_buffered_stream_failing_writes(test *t) {
    uchar bytebuf_buf[24];
    uchar bstream_buf[10];

    struct bytesink_ctx_bytebuf context = {.chunk_size = 4};
    bytebuf_init_fixed(&context.bbuf, bytebuf_buf, 0, sizeof(bytebuf_buf));
    bufstream bstream = {
        .buffer = bstream_buf,
        .cap = sizeof(bstream_buf),
        .len = 0,
        .sink = {
            .fn = bytebuf_collect,
            .context = &context,
        },
    };

    bufstream_write_sstr(&bstream, "this is ");
    bufstream_write_sstr(&bstream, "a somewhat ");

    bufstream_write_result res =
        bufstream_write_sstr(&bstream, "long sentence");
    assert_eq_sint(t, res.err_code, 1, "has error");
    assert_eq_uint(t, res.len, 5, "partial write length");
    assert_eq_bytes(
        t,
        context.bbuf.buffer,
        (const uchar *)"this is a somewhat long ",
        24,
        "sink content full"
    );
}

static test_case tests[] = {
    {"Buffered stream short writes", test_buffered_stream_short_writes},
    {"Buffered stream long writes", test_buffered_stream_long_writes},
    {"Buffered stream failing writes", test_buffered_stream_failing_writes}
};

setup_tests(NULL, tests)
