/**
 * Library of common C utilities i.e. "personal standard library".
 */
#ifndef JP_STD_H
#define JP_STD_H

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Disabling string.h will enable custom "memcpy" and "memmove" functions.
#ifndef JP_DISABLE_STRING_H
#include <string.h>
#endif // JP_DISABLE_STRING_H

////////////////////////
// Scalar types
////////////////////////

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef long long llong;
typedef unsigned long long ullong;

// Boolean

#if __STDC_VERSION__ < 202311L
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef int bool;
#define true 1
#define false 0
#endif
#endif

////////////////////////
// Debugging
////////////////////////

// Breakpoint
#ifdef JP_DEBUG

// Use a builtin breakpoint if possible
#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define breakpoint() __builtin_debugtrap()
#elif __has_builtin(__builtin_trap)
#define breakpoint() __builtin_trap()
#elif __has_builtin(__debugbreak)
#define breakpoint() __debugbreak()
#endif
#endif

// ASM based breakpoint
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

// Fall back to signals for breakpoint
#include <signal.h>
#if defined(SIGTRAP)
#define breakpoint() raise(SIGTRAP)
#else
#define breakpoint() raise(SIGABRT)
#endif

#endif
#endif

#endif // JP_DEBUG

// Assertions
#if defined(JP_USE_ASSERT_H)
#include <assert.h>
#elif defined(JP_DEBUG)

/**
 * Assert: fail when condition does not hold
 */
#ifdef JP_DEBUG_BREAK_ON_ASSERT_FAIL
#define assert(c) \
    while (!(c)) { breakpoint(); }
#else
#define assert(c) \
    while (!(c)) { __builtin_unreachable(); }
#endif

#else
#define assert(c)
#endif // JP_USE_ASSERT_H

// Attribute for ignoring unused code
#if defined(__GNUC__) || defined(__clang__)
#define ignore_unused __attribute__((unused))
#else
#define ignore_unused
#endif

////////////////////////
// Math
////////////////////////

/**
 * Choose the minimum of two numbers
 *
 * @param a,b numbers to compare
 * @returns a or b depending on which is smaller
 */
#define min(a, b) ((a) < (b) ? (a) : (b))

/**
 * Choose the maximum of two numbers
 *
 * @param a,b numbers to compare
 * @returns a or b depending on which is greater
 */
#define max(a, b) ((a) > (b) ? (a) : (b))

/**
 * Limit a number between two numbers.
 *
 * @param n number to limit
 * @param low lower end to limit between
 * @param high high end to limit between
 * @returns a number limited between low and high end
 */
#define clamp(n, low, high) min(max(n, low), high)

/**
 * Convert given number to an absolute value i.e. -N --> N
 *
 * @param a number to convert
 * @returns the number converted to its absolute value
 */
#define abs(a) (((a) >= 0) ? (a) : -(a))

/**
 * Check whether the given number is a power-of-two value
 */
#define is_power_of_two(a) (((a) != 0) && (((a) & ((a) - 1)) == 0))

/**
 * Round up the given number to a multiple of another number.
 *
 * Undefined for multiple of zero.
 *
 * Examples:
 * - 1, 3 --> 3
 * - 5, 3 --> 6
 * - 8, 3 --> 9
 *
 * @param n number to round up
 * @param multiple the multiple of the value to round up to
 * @returns n rounded up to a multiple
 */
ignore_unused static inline ullong
round_up_multiple_ullong(ullong n, ullong multiple) {
    assert(multiple > 0 && "multiple must be >0");
    assert(
        ULLONG_MAX - n > multiple && "n + multiple must not exceed ullong max"
    );

    if (multiple < 2) {
        return n;
    }

    ullong remainder = n % multiple;
    if (remainder == 0) {
        return n;
    }

    return n - remainder + multiple;
}

////////////////////////
// Arrays
////////////////////////

/**
 * Count number of items in an array or a static string
 */
#define countof(x) (sizeof(x) / sizeof(*(x)))

/**
 * Length of a static string
 */
#define lengthof(x) (countof(x) - 1)

////////////////////////
// Alignment
////////////////////////

#if __STDC_VERSION__ < 201112L && defined(__BIGGEST_ALIGNMENT__)
#define max_align_t __BIGGEST_ALIGNMENT__
#endif

#if __STDC_VERSION__ >= 202311L
// >=C23

#include <stdalign.h>
#else
// <C23

/**
 * Get the alignment of given type.
 */
#if __STDC_VERSION__ >= 201112L
#define alignof _Alignof
#elif defined(__GNUC__) || defined(__clang__)
#define alignof __alignof__
#else
#define alignof(x) max_align_t
#endif

/**
 * Get the alignment of given type.
 */
#if __STDC_VERSION__ >= 201112L
#define alignas _Alignas
#elif defined(__GNUC__) || defined(__clang__)
#define alignas(N) __attribute__((__aligned__(N)))
#endif

#endif

/**
 * Align a memory size to the nearest alignment (power-of-two) value.
 *
 * @param size memory size to align
 * @param alignment size to align to (must be power-of-two)
 */
ignore_unused static inline size_t
align_to_nearest(size_t size, size_t alignment) {
    assert(is_power_of_two(alignment) && "alignment must be power of two");
    size_t mask = alignment - 1;
    assert(
        SIZE_MAX - mask > size
        && "size + alignment - 1 must not exceed size max"
    );
    return (size + mask) & ~mask;
}

////////////////////////
// Bits
////////////////////////

#define bitset_is_set(bitset, bit) ((bitset) & (bit))
#define bitset_set(bitset, bit) ((bitset) | (bit))
#define bitset_set_mut(bitset, bit) ((bitset) |= (bit))
#define bitset_unset(bitset, bit) ((bitset) & ~(bit))
#define bitset_unset_mut(bitset, bit) ((bitset) &= ~(bit))

/**
 * Get the position of the most significant bit from the given number.
 *
 * This can be useful for rounding up or down a number to nearest
 * power-of-two number.
 *
 * Since zero does not have a significant bit, the result for it will be
 * undefined.
 *
 * @param n number to check for the most significant bit
 * @returns position of the most significant bit
 */
ignore_unused static inline uint bits_most_significant(ullong n) {
    assert(n > 0 && "n must be >0");

    // On x86, this can get converted to a BSR
    uint i = 0;
    while (n > 0) {
        i += 1;
        n >>= 1;
    }
    return i ? i - 1 : 0;
}

/**
 * Get the number of bits set on left for an unsigned char.
 *
 * @param n number to check the number of bits for
 * @returns count of bits set on left
 */
ignore_unused static inline uint bits_set_on_left_uchar(uchar n) {
    return bits_most_significant(n) ^ (CHAR_BIT - 1);
}

////////////////////////
// Bytes
////////////////////////

#ifndef JP_DISABLE_STRING_H

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

    uchar *d = dest;
    const uchar *s = src;
    for (size_t i = 0; i < n; i += 1) { d[i] = s[i]; }
    return d;
}

/**
 * Basically memmove. Assumes that overlapping can be avoided by comparing
 * pointer ordering, which is OK for targets where this code is used.
 *
 * @param[out] dest area of memory to move bytes to
 * @param[in] src area of memory to move bytes from
 * @param[in] n number of bytes to move
 * @returns pointer to area of memory where bytes were copied to
 */
ignore_unused static inline void *
bytes_move(void *dest, const void *src, size_t n) {
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
ignore_unused static inline void *bytes_set(void *dest, int c, size_t n) {
    if (!n) {
        return dest;
    }
    uchar *d = dest;
    uchar c_ = (uchar)c;
    for (size_t i = 0; i < n; i += 1) { d[i] = c_; }
    return d;
}

#endif // JP_DISABLE_STRING_H

/**
 * Copy a given number of items from one buffer to another
 *
 * @param[out] dest area of memory to copy bytes to
 * @param[in] src area of memory to copy bytes from
 * @param[in] n number of items to copy
 * @returns pointer to area of memory where bytes were copied to
 */
#define copy_n(dest, src, n) bytes_copy((dest), (src), (n) * sizeof(*(src)))

/**
 * Move a given number of items from one buffer to another
 *
 * @param[out] dest area of memory to move bytes to
 * @param[in] src area of memory to move bytes from
 * @param[in] n number of items to move
 * @returns pointer to area of memory where bytes were copied to
 */
#define move_n(dest, src, n) bytes_move((dest), (src), (n) * sizeof(*(src)))

/**
 * Set a given number of bytes to a byte pattern
 *
 * @param[out] dest buffer to fill with a pattern
 * @param[in] c byte pattern to fill the buffer with
 * @param[in] n number of bytes to fill
 * @returns pointer to the memory area that was filled
 */
#define set_n(dest, c, n) bytes_set((dest), (c), (n) * sizeof(*(dest)))

/**
 * Copy a given number of items from one buffer to another
 *
 * @param[out] dest area of memory to copy bytes to
 * @param[in] src area of memory to copy bytes from
 * @param[in] n number of items to copy
 * @param[in] type the type of the buffer items
 * @returns pointer to area of memory where bytes were copied to
 */
#define copy_nt(dest, src, n, type) \
    bytes_copy((dest), (src), (n) * sizeof(type))

/**
 * Move a given number of items from one buffer to another
 *
 * @param[out] dest area of memory to move bytes to
 * @param[in] src area of memory to move bytes from
 * @param[in] n number of items to move
 * @param[in] type the type of the buffer items
 * @returns pointer to area of memory where bytes were copied to
 */
#define move_nt(dest, src, n, type) \
    bytes_move((dest), (src), (n) * sizeof(type))

/**
 * Set a given number of bytes to a byte pattern
 *
 * @param[out] dest buffer to fill with a pattern
 * @param[in] c byte pattern to fill the buffer with
 * @param[in] n number of bytes to fill
 * @param[in] type the type of the buffer items
 * @returns pointer to the memory area that was filled
 */
#define set_nt(dest, c, n, type) bytes_set((dest), (c), (n) * sizeof(type))

/**
 * Check which index the given two buffers differ starting from given index
 *
 * @param a,b byte buffers to compare
 * @param start index to start comparing from
 * @param len the length of the buffers
 * @returns -1 when the buffers contain the same bytes, and the index of the
 * differing byte otherwise
 */
int bytes_diff_index(const void *a, const void *b, size_t start, size_t len);

/**
 * Check whether both buffers contain the same bytes up to the given capacity.
 *
 * @param a,b byte buffers to compare
 * @param len the length of the buffers
 * @returns true when bytes contain the same bytes and false otherwise
 */
bool bytes_eq(const void *a, const void *b, size_t len);

/**
 * Write bytes as a hex string.
 *
 * Note that the destination buffer must have at least twice + 1 the space as
 * the number of bytes that are to be converted.
 *
 * @param[out] dest buffer to write the hex string to
 * @param[in] dest_len length of the dest buffer
 * @param[in] src data to convert to a hex string
 * @param[in] src_len number of bytes to convert to a hex string
 * @returns number of bytes written
 */
size_t
bytes_to_hex(char *dest, size_t dest_len, const char *src, size_t src_len);

/**
 * Result type for any index search.
 */
typedef struct {
    /**
     * Found index
     */
    size_t index;

    /**
     * Whether or not an index was found
     */
    bool ok;
} index_result;

/**
 * Find the index of a byte.
 *
 * @param buffer bytes to search for a byte
 * @param len length of the bytes buffer
 * @param byte byte to search for in the buffer
 * @returns index of the byte and whether or not the byte was found
 */
ignore_unused static inline index_result
bytes_index_of(const uchar *buffer, size_t len, uchar byte) {
    index_result res = {0};
    for (size_t i = 0; i < len; i += 1) {
        if (buffer[i] == byte) {
            res.index = i;
            res.ok = 1;
        }
    }
    return res;
}

////////////////////////
// Slices
////////////////////////

/**
 * Immutable slice (aka fat pointer aka string).
 *
 * Useful for pointing to const strings such as static strings.
 */
typedef struct {
    /**
     * Pointer containing the data
     */
    const uchar *ptr;

    /**
     * Length of the slice in bytes
     */
    size_t len;
} slice_const;

/**
 * Slice (aka fat pointer aka string)
 *
 * Useful for pointing to mutable byte buffers.
 */
typedef struct {
    /**
     * Pointer containing the data
     */
    uchar *ptr;

    /**
     * Length of the slice in bytes
     */
    size_t len;
} slice;

/**
 * Create a slice from an array
 *
 * @param arr an array
 * @returns a new slice
 */
#define slice_arr(arr) \
    (slice) { \
        .ptr = (uchar *)(arr), .len = sizeof(arr), \
    }

/**
 * Create a slice from a mutable string (string array)
 *
 * @param str a string array
 * @returns a new slice
 */
#define slice_str(str) \
    (slice) { \
        .ptr = (uchar *)(str), .len = lengthof(str), \
    }

/**
 * Create a slice from a static string
 *
 * @param str a static string
 * @returns a new slice
 */
#define slice_sstr(str) \
    (slice_const) { \
        .ptr = (const uchar *)(str), .len = lengthof(str), \
    }

/**
 * Create a slice from a pointer to a data and length
 *
 * @param buffer a pointer to data
 * @param len length of the data
 * @returns a new slice
 */
ignore_unused static inline slice slice_new(void *buffer, size_t len) {
    slice s = {
        .ptr = buffer,
        .len = len,
    };
    return s;
}

/**
 * Create a const slice from a pointer to a data and length
 *
 * @param buffer a pointer to data
 * @param len length of the data
 * @returns a new slice
 */
ignore_unused static inline slice_const
slice_const_new(const void *buffer, size_t len) {
    slice_const s = {
        .ptr = buffer,
        .len = len,
    };
    return s;
}

/**
 * Create a slice from a span between two pointers
 *
 * @param start pointer where the data starts from
 * @param end pointer where the data ends
 * @returns slice of data between the given pointers
 */
slice slice_span(void *start, void *end);

/**
 * Check if two slices are equal
 *
 * @param a,b slices to compare
 * @returns true when the slices are equal and false otherwise
 */
bool slice_const_eq(const slice_const a, const slice_const b);

/**
 * Check if two slices are equal
 *
 * @param a,b slices to compare
 * @returns true when the slices are equal and false otherwise
 */
bool slice_eq(const slice a, const slice b);

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
 * Create a const slice from a null terminated string
 *
 * @param str null terminated string
 * @returns a new slice pointing to the given string
 */
slice_const slice_const_from_cstr_unsafe(const char *str);

/**
 * Create a slice from a null terminated string
 *
 * @param str null terminated string
 * @returns a new slice pointing to the given string
 */
slice slice_from_cstr_unsafe(char *str);

/**
 * Check if a slice is set.
 *
 * A slice is considered set if the buffer is not null and
 * the length is greater than zero.
 *
 * @param s slice to check
 * @returns true if the slice is set
 */
ignore_unused static inline bool slice_is_set(slice s) {
    return s.ptr != NULL && s.len != 0;
}

/**
 * Check if a slice is set.
 *
 * A slice is considered set if the buffer is not null and
 * the length is greater than zero.
 *
 * @param s slice to check
 * @returns true if the slice is set
 */
ignore_unused static inline bool slice_const_is_set(slice_const s) {
    return s.ptr != NULL && s.len != 0;
}

////////////////////////
// Allocator
////////////////////////

/**
 * Allocation returned from an allocator
 */
typedef struct {
    /**
     * Pointer to allocated data
     */
    void *ptr;

    /**
     * Length of the data
     */
    const size_t len;
} allocation;

/**
 * Check if an allocation exists in the given value
 */
ignore_unused static inline bool allocation_exists(allocation a) {
    return a.ptr && a.len;
}

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
    allocation (*malloc)(size_t size, size_t alignment, void *ctx);

    /**
     * Function for freeing memory.
     *
     * @param ptr pointer to area of memory to free
     * @param ctx additional data to provide context for freeing memory
     */
    void (*free)(allocation ptr, void *ctx);

    /**
     * Custom data to provide context for the allocator.
     */
    void *ctx;
} allocator;

/**
 * Create given amount of new items of given type using an allocator.
 *
 * @param allocator allocator to use for acquiring memory
 * @param t type of the objects to allocate
 * @param n number of objects to allocate
 * @returns pointer to area of memory that was allocated
 */
#define alloc_new(allocator, t, n) \
    alloc_malloc(allocator, sizeof(t) * (n), alignof(t))

/**
 * Call malloc on a custom memory allocation interface.
 *
 * @param allocator allocator to use for acquiring memory
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation
 * @returns pointer to area of memory that was allocated
 */
ignore_unused static inline allocation
alloc_malloc(allocator *a, size_t size, size_t alignment) {
    return a->malloc(size, alignment, a->ctx);
}

/**
 * Call free on a custom memory allocation interface.
 *
 * @param allocator allocator to use for freeing memory
 * @param ptr pointer to area of memory to free
 */
ignore_unused static inline void alloc_free(allocator *a, allocation ptr) {
    a->free(ptr, a->ctx);
}

/**
 * Standard memory allocation (stdlib malloc) compatible with the custom memory
 * allocation interface.
 *
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation (unused)
 * @param ctx additional data to provide context for the allocation (unused)
 * @returns pointer to area of memory that was allocated
 */
static allocation std_malloc(size_t size, size_t alignment, void *ctx) {
    assert(size > 0 && "size must be greater than 0");
    (void)ctx;
    (void)alignment;
    void *ptr = malloc(size);
    if (ptr == NULL) {
        return (allocation) {0};
    }
    return (allocation) {
        .ptr = ptr,
        .len = size,
    };
}

/**
 * Standard memory freeing (stdlib free) compatible with the custom memory
 * allocation interface.
 *
 * @param ptr pointer to area of memory to free
 * @param ctx additional data to provide context for freeing memory (unused)
 */
static void std_free(allocation a, void *ctx) {
    assert(a.ptr && "ptr must not be null");
    (void)ctx;
    free(a.ptr);
}

/**
 * Standard memory allocation compatible with the custom memory
 * allocation interface.
 */
ignore_unused static allocator std_allocator = {std_malloc, std_free, NULL};

/**
 * Memory allocation using memory mapping compatible with the custom memory
 * allocation interface.
 *
 * @param size amount of memory in bytes to allocate
 * @param alignment memory alignment to use for the allocation (unused)
 * @param ctx additional data to provide context for the allocation (unused)
 * @returns pointer to area of memory that was allocated
 */
allocation mmap_malloc(size_t size, size_t alignment, void *ctx);

/**
 * Memory freeing using memory mapping compatible with the custom memory
 * allocation interface.
 *
 * @param ptr pointer to area of memory to free
 * @param ctx additional data to provide context for freeing memory (unused)
 */
void mmap_free(allocation ptr, void *ctx);

/**
 * Memory allocation using memory mapping compatible with the custom memory
 * allocation interface.
 */
ignore_unused static allocator mmap_allocator = {mmap_malloc, mmap_free, NULL};

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
    uchar *buffer;

    /**
     * Current size of the arena.
     */
    size_t size;

    /**
     * Amount of memory used in the arena.
     */
    size_t used;
} arena;

/**
 * Create a new arena for a backing buffer.
 */
arena arena_new(void *buffer, size_t size);

/**
 * Allocate bytes from the given arena.
 */
void *arena_alloc_bytes(arena *arena, size_t size, size_t alignment);

/**
 * Allocate a number of items of type t from the given arena.
 */
#define arena_alloc(arena, t, count) \
    ((t *)arena_alloc_bytes((arena), sizeof(t) * (count), alignof(t)))

/**
 * Clear the arena usage.
 */
void arena_clear(arena *arena);

/**
 * Custom allocator malloc function for the arena
 */
allocation arena_malloc(size_t size, size_t alignment, void *ctx);

/**
 * Custom allocator free function for the arena
 */
void arena_free(allocation ptr, void *ctx);

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
    ullong len;

    /**
     * Number of items the array can hold
     */
    ullong capacity;

    /**
     * Allocation size in bytes
     *
     * This is used for deallocation
     */
    size_t alloc_size;

    /**
     * Memory allocator used for growing
     */
    allocator *allocator;
} dynarr_header;

/**
 * Convert item count and size to byte size (incl. header)
 *
 * @param n item count
 * @param item_size size of a single item
 * @returns dynarr size in bytes
 */
ignore_unused static inline size_t
dynarr_count_to_bytes(ullong n, size_t item_size) {
    assert(item_size > 0 && "item size must be >0");
    assert(n > 0 && "n must be >0");
    assert(
        (SIZE_MAX - sizeof(dynarr_header)) / item_size > n
        && "n * item size + header size must not exceed size max"
    );
    return (size_t)n * item_size + sizeof(dynarr_header);
}

/**
 * Create a new dynamic array
 */
void *dynarr_new_sized(
    ullong capacity, size_t item_size, size_t alignment, allocator *allocator
);

/**
 * Create a new dynamic array
 */
#define dynarr_new(capacity, t, allocator) \
    ((t *)dynarr_new_sized((capacity), sizeof(t), alignof(t), (allocator)))

/**
 * Get the header for given array
 */
static inline dynarr_header *dynarr_get_header(void *array) {
    if (!array) {
        return NULL;
    }
    return ((dynarr_header *)array) - 1;
}

/**
 * Get the count for given array
 */
ignore_unused static inline ullong dynarr_len(void *array) {
    dynarr_header *header = dynarr_get_header(array);
    if (!header) {
        return 0;
    }
    return header->len;
}

/**
 * Get the capacity for given array
 */
ignore_unused static inline ullong dynarr_capacity(void *array) {
    dynarr_header *header = dynarr_get_header(array);
    if (!header) {
        return 0;
    }
    return header->capacity;
}

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
    void *array, ullong capacity_increase, size_t item_size, size_t alignment
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
    dynarr_grow_ut((array), (capacity_increase), sizeof(t), alignof(t))

/**
 * Clone a given array with new capacity.
 *
 * @returns new array with the original array contents or null when the memory
 * allocation fails
 */
void *dynarr_clone_ut(
    void *array, ullong capacity_increase, size_t item_size, size_t alignment
);

/**
 * Clone a given array with new capacity.
 */
#define dynarr_clone(array, capacity_increase, type) \
    (type *)dynarr_clone_ut( \
        (array), (capacity_increase), sizeof(*(array)), alignof(type) \
    )

/**
 * Push items to given array. Returns true when the operation succeeded (i.e.
 * there's capacity).
 */
bool dynarr_push_ut(
    void *array, const void *items, ullong count, size_t item_size
);

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
    ullong count,
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
        (array), (items), (count), sizeof(type), alignof(type) \
    )

/**
 * Pop an element from the tail of the array.
 *
 * @param[in] array array to pop the value from
 * @param[out] out pointer to write the popped value to
 * @param[in] item_size size of an array item
 * @returns true when an item was popped and false otherwise
 */
bool dynarr_pop_ut(void *array, void *out, size_t item_size);

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
bool dynarr_remove_ut(void *array, ullong index, size_t item_size);

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
bool dynarr_remove_uo_ut(void *array, ullong index, size_t item_size);

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
// UTF-8
////////////////////////

typedef struct {
    uchar b1;
    uchar b2;
    uchar b3;
    uchar b4;
} utf8_rune;

ignore_unused static inline int utf8_char_byte_len(uchar c) {
    uint bits = bits_set_on_left_uchar(c);
    switch (bits) {
    case 0:
        return 1; // ASCII range
    case 1:
        return -2; // Continuation byte
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 4;
    default:
        return -1; // Not valid UTF-8
    }
}

////////////////////////
// C strings
////////////////////////

/**
 * Unsafe equality comparison of two strings.
 *
 * @param s1,s2 strings to compare
 * @returns true when strings are equal and false otherwise
 */
bool cstr_eq_unsafe(const char *s1, const char *s2);

/**
 * Equality comparison of two strings with max bound.
 *
 * @param s1,s2 strings to compare
 * @param len length of the two strings
 * @returns true when strings are equal and false otherwise
 */
bool cstr_eq(const char *s1, const char *s2, size_t len);

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

#define cstr_split_flag_null_terminate (uint)(1)

/**
 * String split iterator
 */
typedef struct {
    /**
     * String to split
     */
    slice str;

    /**
     * Characters based on which string should be split.
     */
    slice_const split_chars;

    /**
     * The next character index to check for string split.
     */
    size_t index;

    /**
     * Flag options for splitting
     */
    uint flags;
} cstr_split_iter;

/**
 * Initialise a split iterator
 *
 * @param[out] split iterator to initialise
 * @param[in] str string to split
 * @param[in] split_chars characters based on which string should be split
 * @param[in] flags flag options for splitting
 */
void cstr_split_init(
    cstr_split_iter *split, slice str, slice_const split_chars, uint flags
);

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
bool cstr_match_wild_ascii(
    const char *txt, size_t txt_len, const char *pat, size_t pat_len
);

/**
 * Unsafe wildcard match for ASCII C strings.
 *
 * @param txt text to match
 * @param pat pattern to match against
 */
bool cstr_match_wild_ascii_unsafe(const char *txt, const char *pat);

size_t cstr_to_int(const char *s, size_t len, int *v);
size_t cstr_to_uint(const char *s, size_t len, uint *v);
size_t cstr_to_llong(const char *s, size_t len, llong *v);
size_t cstr_to_ullong(const char *s, size_t len, ullong *v);
size_t cstr_to_float(const char *s, size_t len, float *v);
size_t cstr_to_double(const char *s, size_t len, double *v);

size_t cstr_from_int(char *dest, size_t len, int src);
size_t cstr_from_uint(char *dest, size_t len, uint src);
size_t cstr_from_llong(char *dest, size_t len, llong src);
size_t cstr_from_ullong(char *dest, size_t len, ullong src);
size_t cstr_from_float(char *dest, size_t len, float src, uint decimals);
size_t cstr_from_double(char *dest, size_t len, double src, uint decimals);

ignore_unused static inline size_t
cstr_from_int_nt(char *dest, size_t len, int src) {
    size_t bytes_written = cstr_from_int(dest, len, src);
    if (bytes_written && bytes_written + 1 < len) {
        dest[bytes_written] = '\0';
        bytes_written += 1;
    }
    return bytes_written;
}

ignore_unused static inline size_t
cstr_from_uint_nt(char *dest, size_t len, uint src) {
    size_t bytes_written = cstr_from_uint(dest, len, src);
    if (bytes_written && bytes_written + 1 < len) {
        dest[bytes_written] = '\0';
        bytes_written += 1;
    }
    return bytes_written;
}

ignore_unused static inline size_t
cstr_from_llong_nt(char *dest, size_t len, llong src) {
    size_t bytes_written = cstr_from_llong(dest, len, src);
    if (bytes_written && bytes_written + 1 < len) {
        dest[bytes_written] = '\0';
        bytes_written += 1;
    }
    return bytes_written;
}

ignore_unused static inline size_t
cstr_from_ullong_nt(char *dest, size_t len, ullong src) {
    size_t bytes_written = cstr_from_ullong(dest, len, src);
    if (bytes_written && bytes_written + 1 < len) {
        dest[bytes_written] = '\0';
        bytes_written += 1;
    }
    return bytes_written;
}

ignore_unused static inline size_t
cstr_from_float_nt(char *dest, size_t len, float src, uint decimals) {
    size_t bytes_written = cstr_from_float(dest, len, src, decimals);
    if (bytes_written && bytes_written + 1 < len) {
        dest[bytes_written] = '\0';
        bytes_written += 1;
    }
    return bytes_written;
}

ignore_unused static inline size_t
cstr_from_double_nt(char *dest, size_t len, double src, uint decimals) {
    size_t bytes_written = cstr_from_double(dest, len, src, decimals);
    if (bytes_written && bytes_written + 1 < len) {
        dest[bytes_written] = '\0';
        bytes_written += 1;
    }
    return bytes_written;
}

size_t cstr_len_int(int src);
size_t cstr_len_uint(uint src);
size_t cstr_len_llong(llong src);
size_t cstr_len_ullong(ullong src);
size_t cstr_len_float(float src, uint decimals);
size_t cstr_len_double(double src, uint decimals);

typedef struct {
    double v;
    uint precision;
} cstr_fmt_float;

typedef struct {
    size_t len;
    bool ok;
} cstr_fmt_result;

cstr_fmt_result cstr_fmt_va(
    char *restrict dest,
    size_t len,
    const char *restrict format,
    va_list va_args
);

ignore_unused static inline cstr_fmt_result
cstr_fmt(char *restrict dest, size_t len, const char *restrict format, ...) {
    va_list va_args;
    va_start(va_args, format);
    cstr_fmt_result res = cstr_fmt_va(dest, len, format, va_args);
    va_end(va_args);
    return res;
}

size_t cstr_fmt_len_va(const char *restrict format, va_list va_args);

ignore_unused static inline size_t
cstr_fmt_len(const char *restrict format, ...) {
    va_list va_args;
    va_start(va_args, format);
    size_t bytes_written = cstr_fmt_len_va(format, va_args);
    va_end(va_args);
    return bytes_written;
}

////////////////////////
// Byte buffer
////////////////////////

typedef struct {
    uchar *buffer;
    size_t len;
    size_t cap;
    allocator *allocator;
} bytebuf;

typedef struct {
    size_t len;
    size_t offset;
    bool ok;
} bytebuf_result;

void bytebuf_init(bytebuf *bbuf, size_t capacity, allocator *allocator);

void bytebuf_init_fixed(bytebuf *bbuf, slice buffer, size_t offset);

ignore_unused static inline bytebuf
bytebuf_new(size_t capacity, allocator *allocator) {
    bytebuf bbuf;
    bytebuf_init(&bbuf, capacity, allocator);
    return bbuf;
}

ignore_unused static inline bytebuf
bytebuf_new_fixed(slice buffer, size_t offset) {
    bytebuf bbuf;
    bytebuf_init_fixed(&bbuf, buffer, offset);
    return bbuf;
}

ignore_unused static inline bool bytebuf_is_init(bytebuf *bbuf) {
    assert(bbuf && "bytebuf must not be null");
    return bbuf && bbuf->buffer && bbuf->cap;
}

ignore_unused static inline bool bytebuf_is_growable(bytebuf *bbuf) {
    assert(bbuf && "bytebuf must not be null");
    return bbuf && (bbuf->allocator != NULL);
}

ignore_unused static inline void bytebuf_clear(bytebuf *bbuf) {
    assert(bbuf && "bytebuf must not be null");
    bbuf->len = 0;
}

ignore_unused static inline void bytebuf_clear_last(bytebuf *bbuf, size_t len) {
    assert(bbuf && "bytebuf must not be null");
    if (bbuf->len > len) {
        bbuf->len -= len;
    }
    bbuf->len = 0;
}

ignore_unused static inline size_t bytebuf_bytes_available(bytebuf *bbuf) {
    assert(bbuf && "bytebuf must not be null");
    return bbuf->cap - bbuf->len;
}

void bytebuf_free(bytebuf *bbuf);

bool bytebuf_grow(bytebuf *bbuf, size_t capacity_increase);

bytebuf bytebuf_clone(bytebuf *bbuf, size_t capacity_increase);

bytebuf_result bytebuf_skip(bytebuf *bbuf, size_t len);
bytebuf_result bytebuf_fill(bytebuf *bbuf, uchar pattern, size_t len);

bytebuf_result bytebuf_write(bytebuf *bbuf, const void *src, size_t len);

bytebuf_result bytebuf_write_int(bytebuf *bbuf, int src);
bytebuf_result bytebuf_write_uint(bytebuf *bbuf, uint src);
bytebuf_result bytebuf_write_llong(bytebuf *bbuf, llong src);
bytebuf_result bytebuf_write_ullong(bytebuf *bbuf, ullong src);
bytebuf_result bytebuf_write_float(bytebuf *bbuf, float src, uint decimals);
bytebuf_result bytebuf_write_double(bytebuf *bbuf, double src, uint decimals);

ignore_unused static inline bytebuf_result
bytebuf_write_str(bytebuf *bbuf, const char *str, size_t len) {
    return bytebuf_write(bbuf, str, len);
}

#define bytebuf_write_sstr(bbuf, str) \
    bytebuf_write_str((bbuf), (str), lengthof(str))

bytebuf_result
bytebuf_fmt_va(bytebuf *bbuf, const char *restrict format, va_list va_args);

ignore_unused static inline bytebuf_result
bytebuf_fmt(bytebuf *bbuf, const char *restrict format, ...) {
    va_list va_args;
    va_start(va_args, format);
    bytebuf_result res = bytebuf_fmt_va(bbuf, format, va_args);
    va_end(va_args);
    return res;
}

////////////////////////
// Buffered byte stream
////////////////////////

typedef struct {
    size_t len;
    int err_code;
} bytesink_result;

typedef struct {
    bytesink_result (*fn)(void *, const uchar *, size_t);
    void *context;
} bytesink;

typedef struct {
    uchar *buffer;
    size_t len;
    size_t cap;
    bytesink sink;
} bufstream;

typedef struct {
    size_t len;
    int err_code;
} bufstream_write_result;

bytesink_result bufstream_flush(bufstream *bstream);

bufstream_write_result
bufstream_write(bufstream *bstream, const void *src, size_t len);

ignore_unused static inline bufstream_write_result
bufstream_write_str(bufstream *bstream, const char *src, size_t len) {
    return bufstream_write(bstream, src, len);
}

#define bufstream_write_sstr(bstream, str) \
    bufstream_write_str((bstream), (str), lengthof(str))

bufstream_write_result bufstream_write_int(bufstream *bstream, int src);
bufstream_write_result bufstream_write_uint(bufstream *bstream, uint src);
bufstream_write_result bufstream_write_llong(bufstream *bstream, llong src);
bufstream_write_result bufstream_write_ullong(bufstream *bstream, ullong src);
bufstream_write_result
bufstream_write_float(bufstream *bstream, float src, uint decimals);
bufstream_write_result
bufstream_write_double(bufstream *bstream, double src, uint decimals);

bufstream_write_result bufstream_fmt_va(
    bufstream *bstream, const char *restrict format, va_list va_args
);

ignore_unused static inline bufstream_write_result
bufstream_fmt(bufstream *bstream, const char *restrict format, ...) {
    va_list va_args;
    va_start(va_args, format);
    bufstream_write_result res = bufstream_fmt_va(bstream, format, va_args);
    va_end(va_args);
    return res;
}

#endif // JP_STD_H
