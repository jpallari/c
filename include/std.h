/**
 * Library of common C utilities i.e. "personal standard library".
 */
#ifndef JP_STD_H
#define JP_STD_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
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

#ifdef JP_DEBUG
// Breakpoint: Use a builtin if possible
#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define breakpoint() __builtin_debugtrap()
#elif __has_builtin(__debugbreak)
#define breakpoint() __debugbreak()
#endif
#endif

// Breakpoint: ASM based
#if !defined(breakpoint)
#if defined(__i386__) || defined(__x86_64__)
static inline void breakpoint(void) {
    __asm__ __volatile__("int3");
}
#elif defined(__aarch64__)
static inline void breakpoint(void) {
    __asm__ __volatile__(".inst 0xd4200000");
}
#elif defined(__arm__)
static inline void breakpoint(void) {
    __asm__ __volatile__(".inst 0xe7f001f0");
}
#else
// Breakpoint: Use signals
#include <signal.h>
#if defined(SIGTRAP)
#define breakpoint() raise(SIGTRAP)
#else
#define breakpoint() raise(SIGABRT)
#endif
#endif
#endif
#endif // JP_DEBUG

#if defined(JP_USE_ASSERT_H)
#include <assert.h>
#elif defined(JP_DEBUG)

/**
 * Assert: fail when condition does not hold
 */
#define assert(c) \
    while (!(c)) __builtin_unreachable()

#else
#define assert(c)
#endif // JP_USE_ASSERT_H

////////////////////////
// Math
////////////////////////

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define abs(a) (((a) >= 0) ? (a) : -(a))

////////////////////////
// Arrays
////////////////////////

/**
 * Count number of items in an array or a static string
 */
#define countof(x) (sizeof(x) / sizeof(*(x)))

/**
 * Length of an array or a static string
 */
#define lengthof(x) (countof(x) - 1)

////////////////////////
// Bytes
////////////////////////

#ifdef JP_USE_STRING_H

/**
 * Basically memcpy
 */
#define bytes_copy memcpy

/**
 * Basically memmove
 */
#define bytes_move memmove

/**
 * Basically memset
 */
#define bytes_set(s, c, n) memset(s, c, n)

#else

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
bytes_copy(void *restrict dest, const void *restrict src, size_t n) {
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
static inline void *bytes_move(void *dest, const void *src, size_t n) {
    assert(dest && "dest must not be null");
    assert(src && "src must not be null");

    if (dest == src) {
        return dest;
    }
    uintptr_t diff = dest < src ? (uintptr_t)src - (uintptr_t)dest
                                : (uintptr_t)dest - (uintptr_t)src;
    if ((size_t)diff > n) {
        bytes_copy(dest, src, n);
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

/**
 * Basically memset.
 *
 * @param[out] dest buffer to fill with a pattern
 * @param[in] c byte pattern to fill the buffer with
 * @param[in] n number of bytes to fill
 * @returns pointer to the memory area that was filled
 */
static inline void *bytes_set(void *dest, int c, size_t n) {
    if (!n) {
        return dest;
    }
    u8 *d = dest;
    for (size_t i = 0; i < n; i += 1) { d[i] = (u8)c; }
    return d;
}

#endif // JP_USE_STRING_H

#define copy_n(dest, src, n) bytes_copy((dest), (src), (n) * sizeof(*(src)))

#define move_n(dest, src, n) bytes_move((dest), (src), (n) * sizeof(*(src)))

#define set_n(dest, c, n) bytes_set((dest), (c), (n) * sizeof(*(dest)))

#define copy_nt(dest, src, n, type) \
    bytes_copy((dest), (src), (n) * sizeof(type))

#define move_nt(dest, src, n, type) \
    bytes_move((dest), (src), (n) * sizeof(type))

#define set_nt(dest, c, n, type) bytes_set((dest), (c), (n) * sizeof(type))

/**
 * Check which index the given two buffers differ starting from given index
 *
 * @param a,b byte buffers to compare
 * @param start index to start comparing from
 * @param capacity the capacity of the buffers
 * @returns -1 when the buffers contain the same bytes, and the index of the
 * differing byte otherwise
 */
int bytes_diff_index(
    const void *a, const void *b, size_t start, size_t capacity
);

/**
 * Check whether both buffers contain the same bytes up to the given capacity.
 *
 * @param a,b byte buffers to compare
 * @param capacity the capacity of the buffers
 * @returns true when bytes contain the same bytes and false otherwise
 */
b32 bytes_eq(const void *a, const void *b, size_t capacity);

/**
 * Write bytes as a hex string.
 *
 * Note that the destination buffer must have at least twice + 1 the space as
 * the number of bytes that are to be converted.
 *
 * @param[out] dest buffer to write the hex string to
 * @param[in] src data to convert to a hex string
 * @param[in] n number of bytes to convert to a hex string
 * @returns number of bytes written
 */
size_t bytes_to_hex(char *dest, const char *src, size_t n);

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
} allocator;

/**
 * Standard memory allocation (stdlib malloc) compatible with the custom memory
 * allocation interface.
 *
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation (unused)
 * @param ctx additional data to provide context for the allocation (unused)
 * @returns pointer to area of memory that was allocated
 */
static void *std_malloc(size_t size, size_t alignment, void *ctx) {
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
static void std_free(void *ptr, void *ctx) {
    (void)ctx;
    free(ptr);
}

/**
 * Standard memory allocation compatible with the custom memory
 * allocation interface.
 */
__attribute__((unused)) static allocator std_allocator = {
    std_malloc, std_free, NULL
};

/**
 * Create given amount of new items of given type using an allocator.
 *
 * @param allocator allocator to use for acquiring memory
 * @param t type of the objects to allocate
 * @param n number of objects to allocate
 * @returns pointer to area of memory that was allocated
 */
#define alloc_new(allocator, t, n) \
    (t *)alloc_malloc(allocator, sizeof(t) * (n), _Alignof(t))

/**
 * Call malloc on a custom memory allocation interface.
 *
 * @param allocator allocator to use for acquiring memory
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation
 * @returns pointer to area of memory that was allocated
 */
#define alloc_malloc(allocator, size, alignment) \
    (allocator)->malloc((size), (alignment), (allocator)->ctx)

/**
 * Call free on a custom memory allocation interface.
 *
 * @param allocator allocator to use for freeing memory
 * @param ptr pointer to area of memory to free
 */
#define alloc_free(allocator, ptr) (allocator)->free((ptr), (allocator)->ctx)

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
    size_t len;
} slice;

/**
 * Create a slice from an array or a static string
 *
 * @param x an array or a static string
 * @returns a new slice
 */
#define slice_from(x) \
    (slice) { \
        (u8 *)(x), sizeof(x) \
    }

/**
 * Create a slice from a span between two pointers
 *
 * @param start pointer where the data starts from
 * @param end pointer where the data ends
 * @returns slice of data between the given pointers
 */
slice slice_span(u8 *start, u8 *end);

/**
 * Check if two slices are equal
 *
 * @param a,b slices to compare
 * @returns true when the slices are equal and false otherwise
 */
s32 slice_eq(const slice a, const slice b);

/**
 * Copy slice contents to another slice where the slices do not overlap.
 *
 * @param[out] dest slice to copy data to
 * @param[in] src slice to copy data from
 */
void slice_copy(slice dest, const slice src);

/**
 * Copy slice contents to another slice where the slices may overlap.
 *
 * @param[out] dest slice to copy data to
 * @param[in] src slice to copy data from
 */
void slice_move(slice dest, const slice src);

/**
 * Create a slice from a null terminated string
 *
 * @param str null terminated string
 * @returns a new slice pointing to the given string
 */
slice slice_from_cstr_unsafe(char *str);

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
} arena;

/**
 * Create a new arena for a backing buffer.
 */
arena arena_new(u8 *buffer, u64 size);

/**
 * Calculate used bytes based on desired usage and alignment.
 */
#define arena_aligned_used(used, alignment) \
    ((used) + (alignment) - 1) & ~((alignment) - 1)

/**
 * Allocate bytes from the given arena.
 */
void *arena_alloc_bytes(arena *arena, u64 size, u64 alignment);

/**
 * Allocate a number of items of type t from the given arena.
 */
#define arena_alloc(arena, t, count) \
    ((t *)arena_alloc_bytes((arena), sizeof(t) * (count), _Alignof(t)))

/**
 * Clear the arena usage.
 */
void arena_clear(arena *arena);

/**
 * Custom allocator malloc function for the arena
 */
void *arena_malloc(size_t size, size_t alignment, void *ctx);

/**
 * Custom allocator free function for the arena
 */
void arena_free(void *ptr, void *ctx);

/**
 * Custom allocator for a memory arena
 */
allocator arena_allocator_new(arena *arena);

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
    u64 len;

    /**
     * Number of items the array can hold
     */
    u64 capacity;

    /**
     * Memory allocator used for growing
     */
    allocator *allocator;
} dynarr_header;

/**
 * Convert item count and size to byte size (incl. header)
 */
#define dynarr_count_to_bytes(n, item_size) \
    (((size_t)(n)) * item_size + sizeof(dynarr_header))

/**
 * Create a new dynamic array
 */
void *dynarr_new_sized(
    u64 capacity, size_t item_size, size_t alignment, allocator *allocator
);

/**
 * Create a new dynamic array
 */
#define dynarr_new(capacity, t, allocator) \
    ((t *)dynarr_new_sized((capacity), sizeof(t), _Alignof(t), (allocator)))

/**
 * Get the header for given array
 */
#define dynarr_get_header(array) (array ? ((dynarr_header *)(array)) - 1 : NULL)

/**
 * Get the count for given array
 */
#define dynarr_len(array) (array ? (((dynarr_header *)(array)) - 1)->len : 0)

/**
 * Get the capacity for given array
 */
#define dynarr_capacity(array) \
    (array ? (((dynarr_header *)(array)) - 1)->capacity : 0)

/**
 * Free the given array
 */
void dynarr_free(void *array);

/**
 * Grow an existing array with a capacity increase.
 *
 * Growing is done by allocating more memory using the array's allocator. If the
 * allocated memory is adjacent to the existing array, the existing array is
 * extended to cover the allocated memory.
 *
 * @param array the array to grow
 * @param capacity_increase the number of additional items the new array should
 * hold
 * @param item_size the size of an item
 * @param alignment the memory alignment of an item
 * @returns an array with increased capacity
 */
void *dynarr_grow_ut(
    void *array, u64 capacity_increase, size_t item_size, size_t alignment
);

/**
 * Grow an existing array with a capacity increase.
 *
 * Growing is done by allocating more memory using the array's allocator. If the
 * allocated memory is adjacent to the existing array, the existing array is
 * extended to cover the allocated memory.
 *
 * @param array the array to grow
 * @param capacity_increase the number of additional items the new array should
 * hold
 * @param t type of the item to hold in the array
 * @returns an array with increased capacity
 */
#define dynarr_grow(array, capacity_increase, t) \
    dynarr_grow_ut((array), (capacity_increase), sizeof(t), _Alignof(t))

/**
 * Clone a given array with new capacity.
 *
 * @returns new array with the original array contents or null when the memory
 * allocation fails
 */
void *dynarr_clone_ut(
    void *array, u64 capacity_increase, size_t item_size, size_t alignment
);

/**
 * Clone a given array with new capacity.
 */
#define dynarr_clone(array, capacity_increase, type) \
    (type *)dynarr_clone_ut( \
        (array), (capacity_increase), sizeof(*(array)), _Alignof(type) \
    )

/**
 * Push items to given array. Returns true when the operation succeeded (i.e.
 * there's capacity).
 */
b32 dynarr_push_ut(void *array, const void *items, u64 count, size_t item_size);

/**
 * Push items to given array.
 *
 * Push may fail under these conditions:
 * - Array is NULL
 * - There is not enough capacity to hold the items
 *
 * @returns false when the operation succeeds and false otherwise
 */
#define dynarr_push(array, items, count) \
    dynarr_push_ut((array), (items), (count), sizeof(*(items)))

/**
 * Push items to given array and grow the array automatically.
 *
 * Push may fail under these conditions:
 * - Array is NULL
 * - Array growth fails
 *
 * @returns array containing the new contents or null when push fails
 */
void *dynarr_push_grow_ut(
    void *array,
    const void *items,
    u64 count,
    size_t item_size,
    size_t alignment
);

/**
 * Push items to given array and grow the array automatically.
 *
 * Push may fail under these conditions:
 * - Array is NULL
 * - Array growth fails
 *
 * @returns array containing the new contents or null when push fails
 */
#define dynarr_push_grow(array, items, count, type) \
    (type *)dynarr_push_grow_ut( \
        (array), (items), (count), sizeof(type), _Alignof(type) \
    )

/**
 * Pop an element from the tail of the array.
 *
 * @param[in] array array to pop the value from
 * @param[out] out pointer to write the popped value to
 * @param[in] item_size size of an array item
 * @returns true when an item was popped and false otherwise
 */
b32 dynarr_pop_ut(void *array, void *out, size_t item_size);

/**
 * Pop an element from the tail of the array
 */
#define dynarr_pop(array, out) dynarr_pop_ut((array), &(out), sizeof(*(array)))

/**
 * Remove an element by index from given array.
 *
 * @param array array remove the value from
 * @param index the index of the element to remove
 * @param item_size size of an array item
 * @returns true when an item was removed and false otherwise
 */
b32 dynarr_remove_ut(void *array, u64 index, size_t item_size);

/**
 * Remove an element by index from given array.
 *
 * @param array array remove the value from
 * @param index the index of the element to remove
 * @returns true when an item was removed and false otherwise
 */
#define dynarr_remove(array, index) \
    dynarr_remove_ut((array), (index), sizeof(*(array)))

/**
 * Remove an element by index from given array, but do not guarantee array
 * element ordering after remove.
 *
 * @param array array remove the value from
 * @param index the index of the element to remove
 * @param item_size size of an array item
 * @returns true when an item was removed and false otherwise
 */
b32 dynarr_remove_uo_ut(void *array, u64 index, size_t item_size);

/**
 * Remove an element by index from given array, but do not guarantee array
 * element ordering after remove.
 *
 * @param array array remove the value from
 * @param index the index of the element to remove
 * @returns true when an item was removed and false otherwise
 */
#define dynarr_remove_uo(array, index) \
    dynarr_remove_uo_ut((array), (index), sizeof(*(array)))

////////////////////////
// C strings
////////////////////////

/**
 * Unsafe equality comparison of two strings.
 *
 * @param s1,s2 strings to compare
 * @returns true when strings are equal and false otherwise
 */
b32 cstr_eq_unsafe(const char *s1, const char *s2);

/**
 * Equality comparison of two strings with max bound.
 *
 * @param s1,s2 strings to compare
 * @param len length of the two strings
 * @returns true when strings are equal and false otherwise
 */
b32 cstr_eq(const char *s1, const char *s2, size_t len);

/**
 * Unsafe get length of string.
 *
 * @param str string to get length for
 * @returns the length of the string
 */
size_t cstr_len_unsafe(const char *str);

/**
 * Get length of string with a bound check.
 *
 * @param str string to get length for
 * @param capacity the capacity of the string
 * @returns the length of the string
 */
size_t cstr_len(const char *str, size_t capacity);

/**
 * String split iterator
 */
typedef struct {
    /**
     * String to split
     */
    char *str;

    /**
     * Length of the string to split
     */
    size_t str_len;

    /**
     * Characters based on which string should be split.
     */
    const char *split_chars;

    /**
     * Length of the split characters list.
     */
    size_t split_chars_len;

    /**
     * The next character index to check for string split.
     */
    size_t index;

    /**
     * Flag: whether or not to null-terminate on split characters
     */
    u32 null_terminate : 1;
} cstr_split_iter;

/**
 * Get a slice to the next sub-string from a split iterator.
 *
 * @param split the split iterator to advance
 * @returns slice to the next sub-string or empty slice when there are no
 * sub-strings left
 */
slice cstr_split_next(cstr_split_iter *split);

/**
 * Collect all sub-strings from a split iterator to an array of slices.
 *
 * @param arr an array of slices to collect sub-strings to
 * @param len length of the array
 * @param split the split iterator to advance
 * @returns number of sub-strings captured from the split iterator
 */
size_t cstr_split_collect(slice *arr, size_t len, cstr_split_iter *split);

/**
 * Collect all sub-strings from a split iterator to an array of C strings.
 *
 * Null-termination will be automatically enabled for the split iterator for the
 * duration of the function.
 *
 * @param arr an array of C-strings to collect sub-strings to
 * @param len length of the array
 * @param split the split iterator to advance
 * @returns number of sub-strings captured from the split iterator
 */
size_t
cstr_split_collect_strings(char **strings, size_t len, cstr_split_iter *split);

/**
 * Wildcard match for ASCII C strings.
 *
 * @param txt text to match
 * @param txt_len length of the text
 * @param pat pattern to match against
 * @param pat_len length of the pattern
 */
b32 cstr_match_wild_ascii(
    const char *txt, size_t txt_len, const char *pat, size_t pat_len
);

/**
 * Unsafe wildcard match for ASCII C strings.
 *
 * @param txt text to match
 * @param pat pattern to match against
 */
b32 cstr_match_wild_ascii_unsafe(const char *txt, const char *pat);

b32 cstr_to_s8(const char *s, size_t len, s8 *v);
b32 cstr_to_u8(const char *s, size_t len, u8 *v);
b32 cstr_to_s16(const char *s, size_t len, s16 *v);
b32 cstr_to_u16(const char *s, size_t len, u16 *v);
b32 cstr_to_s32(const char *s, size_t len, s32 *v);
b32 cstr_to_u32(const char *s, size_t len, u32 *v);
b32 cstr_to_s64(const char *s, size_t len, s64 *v);
b32 cstr_to_u64(const char *s, size_t len, u64 *v);
b32 cstr_to_float(const char *s, size_t len, float *v);
b32 cstr_to_double(const char *s, size_t len, double *v);

size_t cstr_from_s8(char *dest, size_t len, s8 src);
size_t cstr_from_u8(char *dest, size_t len, u8 src);
size_t cstr_from_s16(char *dest, size_t len, s16 src);
size_t cstr_from_u16(char *dest, size_t len, u16 src);
size_t cstr_from_s32(char *dest, size_t len, s32 src);
size_t cstr_from_u32(char *dest, size_t len, u32 src);
size_t cstr_from_s64(char *dest, size_t len, s64 src);
size_t cstr_from_u64(char *dest, size_t len, u64 src);
size_t cstr_from_float(char *dest, size_t len, float src, u8 decimals);
size_t cstr_from_double(char *dest, size_t len, double src, u8 decimals);

#endif // JP_STD_H
