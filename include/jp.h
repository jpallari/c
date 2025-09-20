/**
 * Library of common C utilities i.e. "personal standard library".
 * Naming convention copied from stb.h. :^)
 */
#ifndef JP_H
#define JP_H

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
    assert(dest && "dest must not be null");
    assert(src && "src must not be null");

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
    assert(dest && "dest must not be null");
    assert(src && "src must not be null");

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
        for (size_t i = n; i > 0; i -= 1) { d[n - i] = s[n - i]; }
    } else {
        for (size_t i = n; i > 0; i -= 1) { d[i - 1] = s[i - 1]; }
    }
    return d;
}

#endif // JP_USE_STRING_H

/**
 * Check whether both buffers contain the same bytes up to the given capacity.
 *
 * @param b1,b2 byte buffers to compare
 * @param capacity the capacity of the buffers
 * @returns true when bytes contain the same bytes and false otherwise
 */
b32 jp_bytes_eq(const u8 *b1, const u8 *b2, size_t capacity);

#define jp_copy_n(dest, src, n, type) \
    jp_bytes_copy((dest), (src), n * sizeof(type))

#define jp_move_n(dest, src, n, type) \
    jp_bytes_move((dest), (src), n * sizeof(type))

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
__attribute__((unused)) static jp_allocator jp_std_allocator = {
    jp_std_malloc, jp_std_free, NULL
};

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
// C strings
////////////////////////

/**
 * Unsafe equality comparison of two strings.
 *
 * @param s1,s2 strings to compare
 * @returns true when strings are equal and false otherwise
 */
b32 jp_cstr_eq_unsafe(const char *s1, const char *s2);

/**
 * Equality comparison of two strings with max bound.
 *
 * @param s1,s2 strings to compare
 * @param capacity minimum capacity of the two strings
 * @returns true when strings are equal and false otherwise
 */
b32 jp_cstr_eq(const char *s1, const char *s2, size_t capacity);

/**
 * Unsafe get length of string.
 *
 * @param str string to get length for
 * @returns the length of the string
 */
size_t jp_cstr_len_unsafe(const char *str);

/**
 * Get length of string with a bound check.
 *
 * @param str string to get length for
 * @param capacity the capacity of the string
 * @returns the length of the string
 */
size_t jp_cstr_len(const char *str, size_t capacity);

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
 * Create a slice from an array or a static string
 *
 * @param x an array or a static string
 * @returns a new slice
 */
#define jp_slice_from(x) \
    (jp_slice) { \
        (u8 *)(x), sizeof(x) \
    }

/**
 * Create a slice from a span between two pointers
 *
 * @param start pointer where the data starts from
 * @param end pointer where the data ends
 * @returns slice of data between the given pointers
 */
jp_slice jp_slice_span(u8 *start, u8 *end);

/**
 * Check if two slices are equal
 *
 * @param a,b slices to compare
 * @returns true when the slices are equal and false otherwise
 */
s32 jp_slice_equal(jp_slice a, jp_slice b);

/**
 * Copy slice contents to another slice.
 *
 * @param[out] dest slice to copy data to
 * @param[in] src slice to copy data from
 */
void jp_slice_copy(jp_slice dest, jp_slice src);

/**
 * Create a slice from a null terminated string
 *
 * @param str null terminated string
 * @returns a new slice pointing to the given string
 */
jp_slice jp_slice_from_cstr_unsafe(char *str);

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
jp_arena jp_arena_new(u8 *buffer, u64 size);

/**
 * Calculate used bytes based on desired usage and alignment.
 */
#define jp_arena_aligned_used(used, alignment) \
    ((used) + (alignment) - 1) & ~((alignment) - 1)

/**
 * Allocate bytes from the given arena.
 */
void *jp_arena_alloc_bytes(jp_arena *arena, u64 size, u64 alignment);

/**
 * Allocate a number of items of type t from the given arena.
 */
#define jp_arena_alloc(arena, t, count) \
    ((t *)jp_arena_alloc_bytes((arena), sizeof(t) * (count), _Alignof(t)))

/**
 * Clear the arena usage.
 */
void jp_arena_clear(jp_arena *arena);

/**
 * Custom allocator malloc function for the arena
 */
void *jp_arena_malloc(size_t size, size_t alignment, void *ctx);

/**
 * Custom allocator free function for the arena
 */
void jp_arena_free(void *ptr, void *ctx);

/**
 * Custom allocator for a memory arena
 */
jp_allocator jp_arena_allocator_new(jp_arena *arena);

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
);

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
void jp_dynarr_free(void *array);

/**
 * Clone a given array with new capacity.
 */
void *jp_dynarr_clone_ut(
    void *array, u64 capacity, size_t item_size, size_t alignment
);

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
 * Push items to given array. Returns true when the operation succeeded (i.e.
 * there's capacity).
 */
b32 jp_dynarr_push_ut(void *array, void *items, u64 count, size_t item_size);

/**
 * Push items to given array. Returns true when the operation succeeded (i.e.
 * there's capacity).
 */
#define jp_dynarr_push(array, items, count) \
    jp_dynarr_push_ut((array), (items), (count), sizeof(*(items)))

/**
 * Push items to given array and grow the array automatically. Returns the array
 * with the items.
 */
void *jp_dynarr_push_grow_ut(
    void *array, void *items, u64 count, size_t item_size, size_t alignment
);

/**
 * Push items to given array and assign it back to the array.
 */
#define jp_dynarr_push_grow(array, items, count, type) \
    ((array) = jp_dynarr_push_grow_ut( \
         (array), (items), (count), sizeof(type), _Alignof(type) \
     ))

/**
 * Pop an element from the tail of the array.
 *
 * @param[in] array array to pop the value from
 * @param[out] out pointer to write the popped value to
 * @param[in] item_size size of an array item
 * @returns true when an item was popped and false otherwise
 */
b32 jp_dynarr_pop_ut(void *array, void *out, size_t item_size);

/**
 * Pop an element from the tail of the array
 */
#define jp_dynarr_pop(array, out) \
    (jp_dynarr_pop_ut((array), &(out), sizeof(*(array))))

/**
 * Remove an element by index from given array.
 *
 * @param array array to pop the value from
 * @param item_size size of an array item
 * @returns true when an item was removed and false otherwise
 */
b32 jp_dynarr_remove_ut(void *array, u64 index, size_t item_size);

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

jp_file_result jp_read_file(const char *filename, jp_allocator *allocator);
s64 jp_write_file(char *filename, void *data, u64 size);

#endif // JP_H
