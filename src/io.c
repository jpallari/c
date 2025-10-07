#include "io.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

////////////////////////
// File I/O (blocking)
////////////////////////

file_result read_file(const char *filename, allocator *allocator) {
    assert(filename && "filename must not be null");
    assert(allocator && "allocator must not be null");

    int fd = 0, io_res = 0;
    ssize_t read_res = 0;
    u8 *data = NULL, *cursor = NULL;
    file_result result = {0};
    struct stat file_stat = {0};
    size_t bs_remaining = 0, chunk_size = 0;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        result.err_code = errno;
        return result;
    }

    io_res = fstat(fd, &file_stat);
    if (io_res < 0) {
        result.err_code = errno;
        goto end;
    }
    if (file_stat.st_size == 0) {
        goto end;
    }
    if (file_stat.st_size < 0) {
        result.err_code = -3;
        goto end;
    }
    size_t block_size = (size_t)file_stat.st_blksize;

    data = alloc_new(allocator, u8, (u64)file_stat.st_size);
    if (!data) {
        result.err_code = -2;
        goto end;
    }
    result.data = data;

    for (bs_remaining = (size_t)file_stat.st_size, cursor = data;
         bs_remaining > 0;
         bs_remaining -= (size_t)read_res,
        cursor += read_res,
        result.size += (u64)read_res) {
        chunk_size = min(bs_remaining, block_size);
        read_res = read(fd, cursor, chunk_size);
        if (read_res == 0) { // EOF
            goto end;
        }
        if (read_res < 0) {
            result.err_code = errno;
            goto end;
        }
    }

end:
    io_res = close(fd);
    if (!result.err_code && io_res < 0) {
        result.err_code = errno;
    }

    return result;
}

s64 write_file(char *filename, void *data, u64 size) {
    assert(filename && "filename must not be null");
    assert(data && "data must not be null");

    int fd = 0, io_res = 0, close_res = 0;
    u8 *cursor = data;
    struct stat file_stat = {0};
    size_t bs_remaining = 0, chunk_size = 0;
    ssize_t write_res = 0;

    fd = open(
        filename,
        O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    );
    if (fd < 0) {
        return errno;
    }
    io_res = fstat(fd, &file_stat);
    if (io_res < 0) {
        goto end;
    }
    size_t block_size = (size_t)file_stat.st_blksize;

    for (bs_remaining = size; bs_remaining > 0;
         bs_remaining -= (size_t)write_res, cursor += write_res) {
        chunk_size = min(bs_remaining, block_size);
        write_res = write(fd, cursor, chunk_size);
        if (write_res <= 0) {
            goto end;
        }
    }

end:
    close_res = close(fd);
    if (write_res < 0) {
        return write_res;
    }
    return io_res || close_res || 0;
}
