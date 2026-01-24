/**
 * Library of IO utilities.
 */
#ifndef JP_IO_H
#define JP_IO_H

#include "std.h"

////////////////////////
// File I/O (blocking)
////////////////////////

typedef struct {
    size_t len;
    int err_code;
} io_result;

io_result
io_write_all_sync(int fd, const void *buffer, size_t len, size_t chunk_size);

#define io_write_str_sync(fd, str) \
    io_write_all_sync((fd), (str), sizeof(str), 0)

typedef struct {
    uchar *data;
    size_t len;
    size_t cap;
    int err_code;
} file_read_result;

__attribute__((unused)) static inline void
file_read_result_free(file_read_result res, allocator *allocator) {
    slice s = {
        .buffer = res.data,
        .len = res.cap,
    };
    alloc_free(allocator, s);
}

#define file_err_failed_alloc (int)(-2)
#define file_err_invalid_stat (int)(-3)

file_read_result file_read_sync(const char *filename, allocator *allocator);
io_result file_write_sync(const char *filename, const void *data, size_t len);

typedef struct {
    int fd;
    size_t chunk_size;
} io_file_bytesink_context;

bytesink_result
io_file_bytesink_fn(void *context, const uchar *bytes, size_t len);

__attribute__((unused)) static inline bytesink
io_file_bytesink(io_file_bytesink_context *ctx) {
    bytesink sink = {
        .context = ctx,
        .fn = io_file_bytesink_fn,
    };
    return sink;
}

//////////////////////////////
// STDOUT & STDERR (blocking)
//////////////////////////////

#ifndef JP_IO_STDOUT_BUF_SIZE
#define JP_IO_STDOUT_BUF_SIZE 1024
#endif // JP_IO_STDOUT_BUF_SIZE

#ifndef JP_IO_STDERR_BUF_SIZE
#define JP_IO_STDERR_BUF_SIZE 1024
#endif // JP_IO_STDERR_BUF_SIZE

bufstream *io_stdout_get(void);
bufstream *io_stderr_get(void);
bufstream_write_result io_stdout_write_str(const char *src, size_t len);
bufstream_write_result io_stderr_write_str(const char *src, size_t len);
bufstream_write_result io_stdout_fmt(const char *restrict format, ...);
bufstream_write_result io_stderr_fmt(const char *restrict format, ...);
bytesink_result io_stdout_flush(void);
bytesink_result io_stderr_flush(void);

#define io_stdout_write_sstr(str) io_stdout_write_str((str), lengthof(str))

#define io_stderr_write_sstr(str) io_stderr_write_str((str), lengthof(str))

#endif // JP_IO_H
