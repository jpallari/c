#ifndef JP_H
#define JP_H

#include <stdlib.h>
#include <stdint.h>

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
static inline void jp_bytes_copy(void *dst, void *src, u64 count) {
#ifdef JP_ENABLE_MEMCPY
    memcpy(dst, src, count);
#else
    u8 *d = (u8 *)dst;
    u8 *s = (u8 *)src;
    for (u64 i = 0; i < count; i += 1) {
        d[i] = s[i];
    }
#endif
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
        void *new_array_data = malloc(jp_dynarr_count_to_bytes(
            new_capacity,
            item_size
        ));
        if (!new_array_data) {
            return NULL;
        }

        void *new_array = new_array_data + sizeof(jp_dynarr_header);
        jp_dynarr_header *new_header = (jp_dynarr_header *)new_array_data;
        new_header->count = header->count;
        new_header->capacity = new_capacity;
        jp_bytes_copy(
            new_array,
            array,
            new_header->count*item_size
        );
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

#endif
