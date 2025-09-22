#include "jp.h"
#include <errno.h>

////////////////////////
// Bytes
////////////////////////

b32 jp_bytes_eq(const void *a, const void *b, size_t capacity) {
    const u8 *a_ = a, *b_ = b;

    if (a_ == b_) {
        return 1;
    }
    if (!a || !b) {
        return 0;
    }

    for (size_t i = 0; i < capacity; i += 1) {
        if (a_[i] != b_[i]) {
            return 0;
        }
    }

    return 1;
}

////////////////////////
// C strings
////////////////////////

b32 jp_cstr_eq_unsafe(const char *s1, const char *s2) {
    if (s1 == s2) {
        return 1;
    }
    if (!s1 || !s2) {
        return 0;
    }

    size_t len = 0;
    while (s1[len] && s2[len]) {
        if (s1[len] != s2[len]) {
            return 0;
        }
        len += 1;
    }
    if (s1[len] != s2[len]) {
        return 0;
    }

    return 1;
}

b32 jp_cstr_eq(const char *s1, const char *s2, size_t capacity) {
    if (s1 == s2) {
        return 1;
    }
    if (!s1 || !s2) {
        return 0;
    }

    for (size_t i = 0; i < capacity; i += 1) {
        if (s1[i] != s2[i]) {
            return 0;
        }
        if (!s1[i] && !s2[i]) {
            break;
        }
    }

    return 1;
}

size_t jp_cstr_len_unsafe(const char *str) {
    if (!str) {
        return 0;
    }
    size_t len = 0;
    while (str[len]) { len += 1; }
    len += 1;
    return len;
}

size_t jp_cstr_len(const char *str, size_t capacity) {
    if (!str) {
        return 0;
    }
    size_t len = 0;
    while (str[len] && len < capacity) { len += 1; }
    if (len + 1 < capacity) {
        len += 1;
    }
    return len;
}

////////////////////////
// Slices
////////////////////////

jp_slice jp_slice_span(u8 *start, u8 *end) {
    assert(start && "start must not be null");
    assert(end && "end must not be null");

    u8 *temp;
    if ((uintptr_t)start > (uintptr_t)end) {
        temp = start;
        start = end;
        end = temp;
    }

    jp_slice s = {0};
    s.buffer = start;
    s.len = end - start;
    return s;
}

s32 jp_slice_equal(jp_slice a, jp_slice b) {
    if (a.len != b.len) {
        return 0;
    }
    return jp_bytes_eq(a.buffer, b.buffer, a.len);
}

void jp_slice_copy(jp_slice dest, jp_slice src) {
    size_t amount = min(src.len, dest.len);
    jp_bytes_copy(dest.buffer, src.buffer, amount);
}

void jp_slice_move(jp_slice dest, jp_slice src) {
    size_t amount = min(src.len, dest.len);
    jp_bytes_move(dest.buffer, src.buffer, amount);
}

jp_slice jp_slice_from_cstr_unsafe(char *str) {
    jp_slice slice = {0};
    slice.buffer = (u8 *)str;
    slice.len = jp_cstr_len_unsafe(str);
    return slice;
}

////////////////////////
// Arena allocator
////////////////////////

jp_arena jp_arena_new(u8 *buffer, u64 size) {
    jp_arena arena = {.buffer = buffer, .size = size, .used = 0};
    return arena;
}

void *jp_arena_alloc_bytes(jp_arena *arena, u64 size, u64 alignment) {
    u64 aligned_used = jp_arena_aligned_used(arena->used, alignment);
    if (arena->size < size + aligned_used) {
        assert("Arena ran out of memory");
        return NULL;
    }
    void *p = ((u8 *)arena->buffer) + aligned_used;
    arena->used = aligned_used + size;
    return p;
}

void jp_arena_clear(jp_arena *arena) {
    arena->used = 0;
}

void *jp_arena_malloc(size_t size, size_t alignment, void *ctx) {
    jp_arena *arena = ctx;
    return jp_arena_alloc_bytes(arena, size, alignment);
}

void jp_arena_free(void *ptr, void *ctx) {
    (void)ctx;
    (void)ptr;
}

jp_allocator jp_arena_allocator_new(jp_arena *arena) {
    jp_allocator a = {
        .ctx = arena, .malloc = jp_arena_malloc, .free = jp_arena_free
    };
    return a;
}

////////////////////////
// Dynamic array
////////////////////////

void *jp_dynarr_new_sized(
    u64 capacity, size_t item_size, size_t alignment, jp_allocator *allocator
) {
    u8 *data = jp_malloc(
        jp_dynarr_count_to_bytes(capacity, item_size), alignment, allocator
    );
    if (!data) {
        return NULL;
    }

    jp_dynarr_header *header = (jp_dynarr_header *)data;
    header->len = 0;
    header->capacity = capacity;
    header->allocator = allocator;

    return (data + sizeof(jp_dynarr_header));
}

void jp_dynarr_free(void *array) {
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (!array) {
        return;
    }
    jp_free(header, header->allocator);
}

void *jp_dynarr_clone_ut(
    void *array, u64 capacity, size_t item_size, size_t alignment
) {
    if (!array) {
        return NULL;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    assert(header && "Header must not be null");

    void *new_array =
        jp_dynarr_new_sized(capacity, item_size, alignment, header->allocator);
    if (!new_array) {
        return NULL;
    }

    jp_dynarr_header *new_header = jp_dynarr_get_header(new_array);
    new_header->len = header->len;
    new_header->capacity = capacity;

    jp_bytes_copy(new_array, array, new_header->len * item_size);
    return new_array;
}

b32 jp_dynarr_push_ut(void *array, void *items, u64 count, size_t item_size) {
    if (count == 0) {
        return 0;
    }
    if (!array) {
        return 0;
    }

    jp_dynarr_header *header = jp_dynarr_get_header(array);
    assert(header && "Header must not be null");

    if (header->len + count > header->capacity) {
        // out of capacity
        return 0;
    }

    jp_bytes_copy(
        ((u8 *)array) + header->len * item_size, (u8 *)items, count * item_size
    );
    header->len += count;
    return 1;
}

void *jp_dynarr_push_grow_ut(
    void *array, void *items, u64 count, size_t item_size, size_t alignment
) {
    if (count == 0) {
        return array;
    }

    assert(array && "array must not be null");
    if (!array) {
        return NULL;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    assert(header && "header must not be null");

    if (header->len + count > header->capacity) {
        u64 new_capacity = jp_dynarr_grow_count(header->capacity + count);
        void *new_array =
            jp_dynarr_clone_ut(array, new_capacity, item_size, alignment);
        if (!new_array) {
            return NULL;
        }
        jp_dynarr_header *new_header = jp_dynarr_get_header(new_array);
        assert(new_header && "new header must not be null");
        jp_dynarr_free(array);
        array = new_array;
        header = new_header;
    }

    void *dest = ((u8 *)array) + header->len * item_size;
    jp_bytes_copy(dest, items, count * item_size);
    header->len += count;
    return array;
}

b32 jp_dynarr_pop_ut(void *array, void *out, size_t item_size) {
    if (!array) {
        return 0;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (header->len == 0) {
        return 0;
    }
    u8 *item = ((u8 *)array) + (header->len - 1) * item_size;
    jp_bytes_copy((u8 *)out, item, item_size);
    header->len -= 1;
    return 1;
}

b32 jp_dynarr_remove_ut(void *array, u64 index, size_t item_size) {
    if (!array) {
        return 0;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (header->len <= index) {
        return 0;
    }
    u8 *data = (u8 *)array;
    u8 *data_dst = data + index * item_size;
    u8 *data_src = data_dst + item_size;
    u64 bytes = (header->len - index - 1) * item_size;
    jp_bytes_move(data_dst, data_src, bytes);
    header->len -= 1;
    return 1;
}

////////////////////////
// File I/O (blocking)
////////////////////////

jp_file_result jp_read_file(const char *filename, jp_allocator *allocator) {
    int fd = 0, io_res = 0;
    ssize_t read_res = 0;
    u8 *data = NULL, *cursor = NULL;
    jp_file_result result = {0};
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

    data = jp_new(u8, file_stat.st_size, allocator);
    if (!data) {
        result.err_code = -2;
        goto end;
    }
    result.data = data;

    for (bs_remaining = (size_t)file_stat.st_size, cursor = data;
         bs_remaining > 0;
         bs_remaining -= read_res,
        cursor += read_res,
        result.size += (u64)read_res) {
        chunk_size = min(bs_remaining, file_stat.st_blksize);
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

s64 jp_write_file(char *filename, void *data, u64 size) {
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

    for (bs_remaining = size; bs_remaining > 0;
         bs_remaining -= write_res, cursor += write_res) {
        chunk_size = min(bs_remaining, file_stat.st_blksize);
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
