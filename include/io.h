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
    int err_code;
} file_read_result;

#define file_err_failed_alloc (int)(-2)
#define file_err_invalid_stat (int)(-3)

file_read_result file_read_sync(const char *filename, allocator *allocator);
io_result file_write_sync(const char *filename, const void *data, size_t len);

io_result bytebuf_flush_sync(bytebuf *bbuf, int fd, size_t chunk_size);

typedef struct {
    int fd;
    size_t chunk_size;
} io_file_bytesink_context;

bytesink_result io_file_bytesink(void *context, const uchar *bytes, size_t len);

#endif // JP_IO_H
