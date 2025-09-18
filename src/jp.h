#ifndef JP_H
#define JP_H

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef JP_USE_STRING_H
#include <string.h>
#endif // JP_USE_STRING_H

////////////////////////
// Scalar types
////////////////////////

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;

////////////////////////
// Debugging
////////////////////////

#ifdef JP_USE_ASSERT_H
#include <assert.h>
#elif JP_DEBUG

/**
 * Assert: fail when condition does not hold
 */
#define assert(c) \
    while (!(c)) __builtin_unreachable()

#else
#define assert(c)
#endif // JP_USE_ASSERT_H

////////////////////////
// Arrays
////////////////////////

/**
 * Count number of items in an array or a static string
 */
#define jp_countof(x) (sizeof(x) / sizeof(*(x)))

/**
 * Length of an array or a static string
 */
#define jp_lengthof(x) (jp_countof(x) - 1)

////////////////////////
// Bytes
////////////////////////

#ifdef JP_USE_STRING_H
/**
 * Basically memcpy
 */
#define jp_bytes_copy memcpy

/**
 * Basically memmove
 */
#define jp_bytes_move memmove
#else // JP_USE_STRING_H

/**
 * Basically memcpy. This will most likely be replaced with memcpy during
 * compilation anyway.
 *
 * @param[out] dest area of memory to copy bytes to
 * @param[in] src area of memory to copy bytes from
 * @param[in] n number of bytes to copy
 * @returns pointer to area of memory where bytes were copied to
 */
static inline void *
jp_bytes_copy(void *restrict dest, const void *restrict src, size_t n) {
    char *d = dest;
    const char *s = src;
    for (size_t i = 0; i < n; i += 1) { d[i] = s[i]; }
    return d;
}

/**
 * Basically memmove. Assumes that overlapping can be avoided by comparing
 * pointer ordering, which is OK for targets where this code is used.
 *
 * @param[out] dest area of memory to copy bytes to
 * @param[in] src area of memory to copy bytes from
 * @param[in] n number of bytes to copy
 * @returns pointer to area of memory where bytes were copied to
 */
static inline void *jp_bytes_move(void *dest, const void *src, size_t n) {
    if (dest == src) {
        return dest;
    }
    uintptr_t diff = dest < src ? (uintptr_t)src - (uintptr_t)dest
                                : (uintptr_t)dest - (uintptr_t)src;
    if ((size_t)diff > n) {
        jp_bytes_copy(dest, src, n);
    }

    char *d = dest;
    const char *s = src;

    if (dest < src) {
        for (size_t i = 0; i < n; i += 1) { d[i] = s[i]; }
    } else {
        for (size_t i = n; i > 0; i -= 1) { d[i] = s[i]; }
    }
    return d;
}

#endif // JP_USE_STRING_H

////////////////////////
// Allocator
////////////////////////

/**
 * Interface for memory allocation
 */
typedef struct {
    /**
     * Function for allocating memory.
     *
     * @param size amount of memory in bytes to allocate
     * @param alignment memory alignment to use for the allocation
     * @param ctx additional data to provide context for the allocation
     * @returns pointer to area of memory that was allocated
     */
    void *(*malloc)(size_t size, size_t alignment, void *ctx);

    /**
     * Function for freeing memory.
     *
     * @param ptr pointer to area of memory to free
     * @param ctx additional data to provide context for freeing memory
     */
    void (*free)(void *ptr, void *ctx);

    /**
     * Custom data to provide context for the allocator.
     */
    void *ctx;
} jp_allocator;

/**
 * Standard memory allocation (stdlib malloc) compatible with the custom memory
 * allocation interface.
 *
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation (unused)
 * @param ctx additional data to provide context for the allocation (unused)
 * @returns pointer to area of memory that was allocated
 */
static void *jp_std_malloc(size_t size, size_t alignment, void *ctx) {
    (void)ctx;
    (void)alignment;
    return malloc(size);
}

/**
 * Standard memory allocation (stdlib free) compatible with the custom memory
 * allocation interface.
 *
 * @param ptr pointer to area of memory to free
 * @param ctx additional data to provide context for freeing memory (unused)
 */
static void jp_std_free(void *ptr, void *ctx) {
    (void)ctx;
    free(ptr);
}

/**
 * Standard memory allocation compatible with the custom memory
 * allocation interface.
 */
static jp_allocator jp_std_allocator = {jp_std_malloc, jp_std_free, NULL};

/**
 * Create given amount of new items of given type using an allocator.
 *
 * @param t type of the objects to allocate
 * @param n number of objects to allocate
 * @param allocator allocator to use for acquiring memory
 * @returns pointer to area of memory that was allocated
 */
#define jp_new(t, n, allocator) \
    (t *)jp_malloc(sizeof(t) * (n), _Alignof(t), allocator)

/**
 * Call malloc on a custom memory allocation interface.
 *
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation
 * @param allocator allocator to use for acquiring memory
 * @returns pointer to area of memory that was allocated
 */
#define jp_malloc(size, alignment, allocator) \
    ((allocator)->malloc((size), (alignment), (allocator)->ctx))

/**
 * Call free on a custom memory allocation interface.
 *
 * @param ptr pointer to area of memory to free
 * @param allocator allocator to use for freeing memory
 */
#define jp_free(ptr, allocator) ((allocator)->free((ptr), (allocator)->ctx))

////////////////////////
// Slices
////////////////////////

/**
 * Slice (aka fat pointer aka string)
 */
typedef struct {
    /**
     * Buffer containing the data
     */
    u8 *buffer;

    /**
     * Length of the slice in bytes
     */
    size_t size;
} jp_slice;

/**
 * Create a slice to an array or a static string
 *
 * @param x an array or a static string
 * @returns a new slice
 */
#define jp_slice_from(x) \
    (jp_slice) { \
        (u8 *)(x), jp_lengthof(x) \
    }

/**
 * Create a slice from a span between two pointers
 *
 * @param start pointer where the data starts from
 * @param end pointer where the data ends
 * @returns slice of data between the given pointers
 */
jp_slice jp_slice_span(u8 *start, u8 *end) {
    u8 *temp;
    if ((uintptr_t)start > (uintptr_t)end) {
        temp = start;
        start = end;
        end = temp;
    }

    jp_slice s = {0};
    s.buffer = start;
    s.size = end - start;
    return s;
}

/**
 * Check if two slices are equal
 *
 * @param a,b slices to compare
 * @returns true when the slices are equal and false otherwise
 */
s32 jp_slice_equal(jp_slice a, jp_slice b) {
    if (a.size != b.size) {
        return 0;
    }
    for (size_t i = 0; i < a.size; i += 1) {
        if (a.buffer[i] != b.buffer[i]) {
            return 0;
        }
    }
    return 1;
}

/**
 * Copy slice contents to another slice.
 *
 * @param[out] dest slice to copy data to
 * @param[in] src slice to copy data from
 */
void jp_slice_copy(jp_slice dest, jp_slice src) {
    size_t amount = src.size > dest.size ? dest.size : src.size;
    jp_bytes_move(dest.buffer, src.buffer, amount);
}

////////////////////////
// Arena allocator
////////////////////////

/**
 * Linear memory arena.
 */
typedef struct {
    /**
     * Buffer to back the arena.
     */
    u8 *buffer;

    /**
     * Current size of the arena.
     */
    u64 size;

    /**
     * Amount of memory used in the arena.
     */
    u64 used;
} jp_arena;

/**
 * Create a new arena for a backing buffer.
 */
jp_arena jp_arena_new(u8 *buffer, u64 size) {
    jp_arena arena = {.buffer = buffer, .size = size, .used = 0};
    return arena;
}

/**
 * Calculate used bytes based on desired usage and alignment.
 */
#define jp_arena_aligned_used(used, alignment) \
    ((used) + (alignment) - 1) & ~((alignment) - 1)

/**
 * Allocate bytes from the given arena.
 */
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

/**
 * Allocate a number of items of type t from the given arena.
 */
#define jp_arena_alloc(arena, t, count) \
    ((t *)jp_arena_alloc_bytes((arena), sizeof(t) * (count), _Alignof(t)))

/**
 * Clear the arena usage.
 */
void jp_arena_clear(jp_arena *arena) {
    arena->used = 0;
}

/**
 * Custom allocator malloc function for the arena
 */
void *jp_arena_malloc(size_t size, size_t alignment, void *ctx) {
    jp_arena *arena = ctx;
    return jp_arena_alloc_bytes(arena, size, alignment);
}

/**
 * Custom allocator free function for the arena
 */
void jp_arena_free(void *ptr, void *ctx) {
    (void)ctx;
    (void)ptr;
}

/**
 * Custom allocator for a memory arena
 */
jp_allocator jp_arena_allocator_new(jp_arena *arena) {
    jp_allocator a = {
        .ctx = arena, .malloc = jp_arena_malloc, .free = jp_arena_free
    };
    return a;
}

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

    /**
     * Memory allocator used for growing
     */
    jp_allocator *allocator;
} jp_dynarr_header;

/**
 * Calculate array size to grow to after given size
 */
#define jp_dynarr_grow_count(n) (2 * (n) + 8)

/**
 * Convert item count and size to byte size (incl. header)
 */
#define jp_dynarr_count_to_bytes(n, item_size) \
    (((size_t)(n)) * item_size + sizeof(jp_dynarr_header))

/**
 * Create a new dynamic array
 */
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
    header->count = 0;
    header->capacity = capacity;
    header->allocator = allocator;

    return (data + sizeof(jp_dynarr_header));
}

/**
 * Create a new dynamic array
 */
#define jp_dynarr_new(capacity, t, allocator) \
    ((t *)jp_dynarr_new_sized((capacity), sizeof(t), _Alignof(t), (allocator)))

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
void jp_dynarr_free(void *array) {
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    if (!array) {
        return;
    }
    jp_free(header, header->allocator);
}

/**
 * Clone a given array with new capacity.
 */
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
    new_header->count = header->count;
    new_header->capacity = capacity;

    jp_bytes_copy(new_array, array, new_header->count * item_size);
    return new_array;
}

/**
 * Clone a given array with new capacity.
 */
#define jp_dynarr_clone(array, capacity, t) \
    ((t *)(jp_dynarr_clone_ut( \
        (array), (capacity), sizeof(*(array)), _Alignof(t) \
    )))

/**
 *  Grow a given array.
 */
#define jp_dynarr_grow(array, t) \
    jp_dynarr_clone( \
        (array), jp_dynarr_grow_count(jp_dynarr_get_capacity(array)), t \
    )

/**
 * Push item to given array. Returns true when the operation succeeded (i.e.
 * there's capacity).
 */
b32 jp_dynarr_push_ut(
    void *array, void *item, size_t item_size, size_t aligment
) {
    if (!array) {
        return 0;
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    assert(header && "Header must not be null");

    if (header->count >= header->capacity) {
        // out of capacity
        return 0;
    }

    jp_bytes_copy(
        ((u8 *)array) + header->count * item_size, (u8 *)item, item_size
    );
    header->count += 1;
    return 1;
}

/**
 * Push item to given array. Returns true when the operation succeeded (i.e.
 * there's capacity).
 */
#define jp_dynarr_push(array, item) \
    jp_dynarr_push_ut((array), &(item), sizeof(item), _Alignof(item))

/**
 * Push item to given array and grow the array automatically. Returns the array
 * with the item.
 */
void *jp_dynarr_push_grow_ut(
    void *array, void *item, size_t item_size, size_t alignment
) {
    if (!array) {
        // Array does not exist? Create a new one from scratch.
        // Use standard allocator since previous allocator is unknown.
        array = jp_dynarr_new_sized(
            jp_dynarr_grow_count(0), item_size, alignment, &jp_std_allocator
        );
    }
    jp_dynarr_header *header = jp_dynarr_get_header(array);
    assert(header && "Header must not be null");

    if (header->count >= header->capacity) {
        u64 new_capacity = jp_dynarr_grow_count(header->capacity);
        void *new_array =
            jp_dynarr_clone_ut(array, new_capacity, item_size, alignment);
        if (!new_array) {
            return NULL;
        }
        jp_dynarr_header *new_header = jp_dynarr_get_header(new_array);
        assert(new_header && "New header must not be null");
        jp_dynarr_free(array);
        array = new_array;
        header = new_header;
    }

    jp_bytes_copy(
        ((u8 *)array) + header->count * item_size, (u8 *)item, item_size
    );
    header->count += 1;
    return array;
}

/**
 * Push item to given array and assign it back to the array.
 */
#define jp_dynarr_push_grow(array, item, type) \
    ((array) = jp_dynarr_push_grow_ut( \
         (array), &(item), sizeof(type), _Alignof(type) \
     ))

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
    u8 *data = (u8 *)array;
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
    u8 *data;
    u64 size;
    s32 err_code;
} jp_file_result;

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

#endif // JP_H
