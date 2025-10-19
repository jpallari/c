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
    uchar *data;
    size_t size;
    int err_code;
} file_result;

file_result read_file(const char *filename, allocator *allocator);
ssize_t write_file(char *filename, void *data, size_t size);

#endif // JP_IO_H
