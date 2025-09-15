#ifndef JP_H
#define JP_H

#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;
typedef s32      b32;

////////////////////////
// Bytes
////////////////////////

/**
 * Basically memcpy
 */
#define jp_bytes_copy memcpy

/**
 * Basically memmove
 */
#define jp_bytes_move memmove

////////////////////////
// Dynamic array
////////////////////////

/**
 * Dynamic array header
 */
typedef struct {
    /**
     * Number of existing items
     */
    u64 count;

    /**
     * Number of items the array can hold
     */
    u64 capacity;
} jp_dynarr_header;

/**
 * Calculate array size to grow to after given size
 */
#define jp_dynarr_grow_count(n) (2*(n) + 8)

/**
 * Convert item count and size to byte size (incl. header)
 */
#define jp_dynarr_count_to_bytes(n, item_size) \
    (((size_t)(n)) * item_size + sizeof(jp_dynarr_header))

/**
 * Create a new dynamic array
 */
void *jp_dynarr_new_sized(u64 capacity, size_t item_size) {
    void* data = malloc(jp_dynarr_count_to_bytes(capacity, item_size));
    if (!data) {
        return NULL;
    }

    jp_dynarr_header *header = (jp_dynarr_header *)data;
    header->count = 0;
    header->capacity = capacity;

    return (data + sizeof(jp_dynarr_header));
}

/**
 * Create a new dynamic array
 */
#define jp_dynarr_new(capacity, t) \
    ((t *)jp_dynarr_new_sized(capacity, sizeof(t)))

/**
 * Get the header for given array
 */
#define jp_dynarr_get_header(array) \
    (array ? ((jp_dynarr_header *)(array)) - 1 : NULL)

/**
 * Get the count for given array
 */
#define jp_dynarr_get_count(array) \
    (array ? (((jp_dynarr_header *)(array)) - 1)->count : 0)

/**
 * Get the capacity for given array
 */
#define jp_dynarr_get_capacity(array) \
    (array ? (((jp_dynarr_header *)(array)) - 1)->capacity : 0)

/**
 * Free the given array
 */
#define jp_dynarr_free(array) \
    (array ? free(((jp_dynarr_header *)(array)) - 1) : NULL)

/**
 * Clone a given array with new capacity.
 */
void *jp_dynarr_clone_ut(void *array, u64 capacity, size_t item_size) {
    if (!array) {
        return NULL;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (!header) {
        return NULL;
    }

    void *new_array = jp_dynarr_new_sized(capacity, item_size);
    if (!new_array) {
        return NULL;
    }

    jp_dynarr_header *new_header = jp_dynarr_get_header(new_array);
    new_header->count = header->count;
    new_header->capacity = capacity;

    jp_bytes_copy(
        new_array,
        array,
        new_header->count*item_size
    );

    return new_array;
}

/**
 * Clone a given array with new capacity.
 */
#define jp_dynarr_clone(array, capacity, t) \
    ((t *)(jp_dynarr_clone_ut((array), (capacity), sizeof(*(array)))))

/**
 * Push item to given array. Returns the array with the item.
 */
void *jp_dynarr_push_ut(void *array, void *item, size_t item_size) {
    if (!array) {
        array = jp_dynarr_new_sized(
            jp_dynarr_grow_count(8),
            item_size
        );
    }

    jp_dynarr_header *header = jp_dynarr_get_header(array);

    if (header->count >= header->capacity) {
        u64 new_capacity = jp_dynarr_grow_count(header->capacity);
        void *new_array = jp_dynarr_clone_ut(array, new_capacity, item_size);
        if (!new_array) {
            return NULL;
        }
        jp_dynarr_header *new_header = jp_dynarr_get_header(new_array);
        jp_dynarr_free(array);
        array = new_array;
        header = new_header;
    }

    jp_bytes_copy(
        ((u8 *)array) + header->count * item_size,
        (void *)item,
        item_size
    );
    header->count += 1;
    return array;
}

/**
 * Push item to given array and assign it back to the array.
 */
#define jp_dynarr_push(array, item) \
    ((array) = jp_dynarr_push_ut((array), &(item), sizeof(item)))

/**
 * Pop an element from the tail of the array
 */
void jp_dynarr_pop_ut(void *array, void *out, size_t item_size) {
    if (!array) {
        return;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (header->count == 0) {
        return;
    }
    u8 *item = ((u8 *)array) + (header->count - 1) * item_size;
    jp_bytes_copy((u8 *)out, item, item_size);
    header->count -= 1;
}

/**
 * Pop an element from the tail of the array
 */
#define jp_dynarr_pop(array, out) \
    (jp_dynarr_pop_ut((array), &(out), sizeof(*(array))))

/**
 * Remove an element by index from given array.
 */
void jp_dynarr_remove_ut(void *array, u64 index, size_t item_size) {
    if (!array) {
        return;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (header->count < index) {
        return;
    }
    u8 *data = (u8*)array;
    u8 *data_dst = data + index * item_size;
    u8 *data_src = data_dst + item_size;
    u64 bytes = (header->count - index - 1) * item_size;
    jp_bytes_move(data_dst, data_src, bytes);
    header->count -= 1;
}

/**
 * Remove an element by index from given array.
 */
#define jp_dynarr_remove(array, index) \
    (jp_dynarr_remove_ut((array), (index), sizeof(*(array))))

////////////////////////
// File I/O (blocking)
////////////////////////

typedef struct {
    void *data;
    u64 size;
    s32 err_code;
} jp_file_result;

jp_file_result jp_read_file(const char *filename) {
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

    data = malloc(file_stat.st_size);
    if (!data) {
        result.err_code = -2;
        goto end;
    }
    result.data = data;

    for (
        bs_remaining = (size_t)file_stat.st_size, cursor = data;
        bs_remaining > 0;
        bs_remaining -= read_res, cursor += read_res, result.size += (u64)read_res
    ) {
        chunk_size = (bs_remaining < file_stat.st_blksize)
            ? bs_remaining
            : file_stat.st_blksize;
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

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        return errno;
    }
    io_res = fstat(fd, &file_stat);
    if (io_res < 0) {
        goto end;
    }

    for (
        bs_remaining = size;
        bs_remaining > 0;
        bs_remaining -= write_res, cursor += write_res
    ) {
        chunk_size = (bs_remaining < file_stat.st_blksize)
            ? bs_remaining
            : file_stat.st_blksize;
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

#endif
