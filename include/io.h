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
    u8 *data;
    u64 size;
    s32 err_code;
} file_result;

file_result read_file(const char *filename, allocator *allocator);
s64 write_file(char *filename, void *data, u64 size);

#endif // JP_IO_H
