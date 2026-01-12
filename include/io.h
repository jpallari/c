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
} os_io_result;

os_io_result
os_write_all(int fd, void *buffer, size_t len, size_t chunk_size);

typedef struct {
    uchar *data;
    size_t len;
    int err_code;
} file_read_result;

#define file_err_failed_alloc (int)(-2)
#define file_err_invalid_stat (int)(-3)

file_read_result file_read(const char *filename, allocator *allocator);
os_io_result file_write(const char *filename, void *data, size_t len);

#endif // JP_IO_H
