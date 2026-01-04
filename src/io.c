#include "io.h"
#include "std.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

////////////////////////
// File I/O (blocking)
////////////////////////

io_result os_read_all(int fd, uchar *buffer, size_t len, size_t chunk_len) {
    assert(buffer && "buffer may not be null");
    assert(len > 0 && "len must be > 0");

    io_result res = {0};

    if (len == 0) {
        return res;
    }
    if (chunk_len == 0) {
        chunk_len = len;
    }

    while (res.len < len) {
        size_t next_chunk_len = min(len - res.len, chunk_len);
        ssize_t read_res = read(fd, buffer + res.len, next_chunk_len);
        if (read_res < 0) {
            res.err_code = errno;
            break;
        }
        if (read_res == 0) {
            break;
        }
        res.len += (size_t)read_res;
    }

    return res;
}

io_result io_write_all_sync(int fd, const void *buffer, size_t len, size_t chunk_len) {
    assert(buffer && "buffer may not be null");

    const uchar *buf_ = buffer;
    io_result res = {0};

    if (len == 0) {
        return res;
    }
    if (chunk_len == 0) {
        chunk_len = len;
    }

    while (res.len < len) {
        size_t next_chunk_size = min(len - res.len, chunk_len);
        ssize_t write_res = write(fd, buf_ + res.len, next_chunk_size);
        if (write_res < 0) {
            res.err_code = errno;
            break;
        }
        res.len += (size_t)write_res;
    }

    return res;
}

file_read_result file_read_sync(const char *filename, allocator *allocator) {
    assert(filename && "filename must not be null");
    assert(allocator && "allocator must not be null");

    file_read_result res = {0};

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        res.err_code = errno;
        return res;
    }

    struct stat file_stat = {0};
    int io_res = fstat(fd, &file_stat);
    if (io_res < 0) {
        res.err_code = errno;
        return res;
    }
    if (file_stat.st_size == 0) {
        goto end;
    }
    if (file_stat.st_size < 0 || file_stat.st_blksize < 0) {
        res.err_code = -3;
        goto end;
    }
    size_t block_size = (size_t)file_stat.st_blksize;

    size_t len = (size_t)file_stat.st_size;
    res.data = alloc_new(allocator, uchar, len);
    if (!res.data) {
        res.err_code = -2;
        goto end;
    }

    io_result io_read_res = os_read_all(fd, res.data, len, block_size);
    res.len = io_read_res.len;
    res.err_code = io_read_res.err_code;

end:
    io_res = close(fd);
    if (!res.err_code && io_res < 0) {
        res.err_code = errno;
    }

    return res;
}

io_result file_write_sync(const char *filename, const void *data, size_t len) {
    assert(filename && "filename must not be null");
    assert(data && "data must not be null");

    io_result res = {0};

    int fd = open(
        filename,
        O_WRONLY | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    );
    if (fd < 0) {
        res.err_code = errno;
        return res;
    }

    struct stat file_stat = {0};
    int io_res = fstat(fd, &file_stat);
    if (io_res < 0) {
        res.err_code = errno;
        goto end;
    }
    if (file_stat.st_size || file_stat.st_blksize < 0) {
        res.err_code = -3;
        goto end;
    }

    res = io_write_all_sync(fd, data, len, (size_t)file_stat.st_blksize);

end:
    io_res = close(fd);
    if (res.err_code == 0 && io_res < 0) {
        res.err_code = errno;
    }
    return res;
}

io_result bytebuf_flush_sync(bytebuf *bbuf, int fd, size_t chunk_size) {
    assert(bbuf && "bbuf must not be null");

    io_result res = io_write_all_sync(fd, bbuf->buffer, bbuf->len, chunk_size);
    assert(res.len <= bbuf->len);

    if (res.len < bbuf->len) {
        size_t new_len = bbuf->len - res.len;
        bytes_move(bbuf->buffer, bbuf->buffer + res.len, new_len);
        bbuf->len = new_len;
    } else {
        bbuf->len = 0;
    }

    return res;
}

bytesink_result io_file_bytesink(void *context, const uchar *bytes, size_t len) {
    io_file_bytesink_context *ctx = (io_file_bytesink_context *)context;
    bytesink_result res;
    io_result io_res = io_write_all_sync(ctx->fd, bytes, len, ctx->chunk_size);
    res.len = io_res.len;
    res.err_code = io_res.err_code;
    return res;
}
