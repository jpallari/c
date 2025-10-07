#include "std.h"
#include <errno.h>

////////////////////////
// Bytes
////////////////////////

int bytes_diff_index(
    const void *a, const void *b, size_t start, size_t capacity
) {
    assert(start < capacity && "start must be lower than capacity");
    const u8 *a_ = a, *b_ = b;
    if ((uintptr_t)a_ == (uintptr_t)b_) {
        return -1;
    }
    if (!a || !b) {
        return 0;
    }

    for (size_t i = start; i < capacity; i += 1) {
        if (a_[i] != b_[i]) {
            return (int)i;
        }
    }

    return -1;
}

b32 bytes_eq(const void *a, const void *b, size_t capacity) {
    if (bytes_diff_index(a, b, 0, capacity) >= 0) {
        return 0;
    }
    return 1;
}

char byte_to_hex_char(char b) {
    assert(b < 16 && "byte must be between 0..=15");

    if (b < 10) {
        return '0' + b;
    }
    return (char)((int)'a' + (int)b - 10);
}

void byte_to_hex_chars(char b, char *high, char *low) {
    *high = byte_to_hex_char(b / 16);
    *low = byte_to_hex_char(b % 16);
}

size_t bytes_to_hex(char *dest, const char *src, size_t n) {
    size_t j = 0;
    for (size_t i = 0; i < n; i += 1, j += 2) {
        byte_to_hex_chars(src[i], &dest[j], &dest[j + 1]);
    }
    dest[j] = 0;
    j += 1;
    return j;
}

////////////////////////
// Slices
////////////////////////

slice slice_span(u8 *start, u8 *end) {
    assert(start && "start must not be null");
    assert(end && "end must not be null");

    u8 *temp;
    if ((uintptr_t)start > (uintptr_t)end) {
        temp = start;
        start = end;
        end = temp;
    }

    slice s = {0};
    s.buffer = start;
    s.len = (u64)((uintptr_t)end - (uintptr_t)start);
    return s;
}

s32 slice_eq(const slice a, const slice b) {
    if (a.len != b.len) {
        return 0;
    }
    return bytes_eq(a.buffer, b.buffer, a.len);
}

void slice_copy(slice dest, const slice src) {
    size_t amount = min(src.len, dest.len);
    bytes_copy(dest.buffer, src.buffer, amount);
}

void slice_move(slice dest, const slice src) {
    size_t amount = min(src.len, dest.len);
    bytes_move(dest.buffer, src.buffer, amount);
}

slice slice_from_cstr_unsafe(char *str) {
    slice slice = {0};
    slice.buffer = (u8 *)str;
    slice.len = cstr_len_unsafe(str);
    return slice;
}

////////////////////////
// Arena allocator
////////////////////////

arena arena_new(u8 *buffer, u64 size) {
    arena arena = {.buffer = buffer, .size = size, .used = 0};
    return arena;
}

void *arena_alloc_bytes(arena *arena, u64 size, u64 alignment) {
    u64 aligned_used = arena_aligned_used(arena->used, alignment);
    if (arena->size < size + aligned_used) {
        return NULL;
    }
    void *p = ((u8 *)arena->buffer) + aligned_used;
    arena->used = aligned_used + size;
    return p;
}

void arena_clear(arena *arena) {
    arena->used = 0;
}

void *arena_malloc(size_t size, size_t alignment, void *ctx) {
    arena *arena = ctx;
    return arena_alloc_bytes(arena, size, alignment);
}

void arena_free(void *ptr, void *ctx) {
    (void)ctx;
    (void)ptr;
}

allocator arena_allocator_new(arena *arena) {
    allocator a = {
        .ctx = arena, .malloc = arena_malloc, .free = arena_free
    };
    return a;
}

////////////////////////
// Dynamic array
////////////////////////

void *dynarr_new_sized(
    u64 capacity, size_t item_size, size_t alignment, allocator *allocator
) {
    u8 *data = alloc_malloc(
        allocator, dynarr_count_to_bytes(capacity, item_size), alignment
    );
    if (!data) {
        return NULL;
    }

    dynarr_header *header = (dynarr_header *)data;
    header->len = 0;
    header->capacity = capacity;
    header->allocator = allocator;

    return data + sizeof(dynarr_header);
}

void dynarr_free(void *array) {
    dynarr_header *header = dynarr_get_header(array);
    if (!array) {
        return;
    }
    alloc_free(header->allocator, header);
}

void *dynarr_grow_ut(
    void *array, u64 capacity_increase, size_t item_size, size_t alignment
) {
    if (!array) {
        return NULL;
    }
    dynarr_header *header = dynarr_get_header(array);
    assert(header && "Header must not be null");

    u64 capacity = capacity_increase + header->capacity;
    u8 *new_array_data = alloc_malloc(
        header->allocator,
        dynarr_count_to_bytes(capacity, item_size),
        alignment
    );
    if (!new_array_data) {
        return NULL;
    }

    if ((uintptr_t)array + header->capacity == (uintptr_t)new_array_data) {
        // We got adjacent memory block, so we can extend the array and skip
        // copying. This mainly works with memory arenas.
        // If this happens for other allocators, this could introduce a memory
        // leak because freeing the array will only free one of the pointers.
        header->capacity += capacity;
        return array;
    }

    u8 *new_array = new_array_data + sizeof(dynarr_header);
    dynarr_header *new_header = (dynarr_header *)new_array_data;
    new_header->len = header->len;
    new_header->capacity = capacity;
    new_header->allocator = header->allocator;

    bytes_copy(new_array, array, new_header->len * item_size);
    return new_array;
}

void *dynarr_clone_ut(
    void *array, u64 capacity_increase, size_t item_size, size_t alignment
) {
    if (!array) {
        return NULL;
    }
    dynarr_header *header = dynarr_get_header(array);
    assert(header && "Header must not be null");

    u64 capacity = capacity_increase + header->capacity;
    void *new_array =
        dynarr_new_sized(capacity, item_size, alignment, header->allocator);
    if (!new_array) {
        return NULL;
    }

    dynarr_header *new_header = dynarr_get_header(new_array);
    new_header->len = header->len;
    new_header->capacity = capacity;

    bytes_copy(new_array, array, new_header->len * item_size);
    return new_array;
}

b32 dynarr_push_ut(
    void *array, const void *items, u64 count, size_t item_size
) {
    if (count == 0) {
        return 0;
    }
    if (!array) {
        return 0;
    }

    dynarr_header *header = dynarr_get_header(array);
    assert(header && "Header must not be null");

    if (header->len + count > header->capacity) {
        // out of capacity
        return 0;
    }

    bytes_copy(
        ((u8 *)array) + header->len * item_size, items, count * item_size
    );
    header->len += count;
    return 1;
}

void *dynarr_push_grow_ut(
    void *array,
    const void *items,
    u64 count,
    size_t item_size,
    size_t alignment
) {
    if (count == 0) {
        return array;
    }

    assert(array && "array must not be null");
    if (!array) {
        return NULL;
    }
    dynarr_header *header = dynarr_get_header(array);
    assert(header && "header must not be null");

    if (header->len + count > header->capacity) {
        u64 capacity_increase = header->capacity + count + 8;
        void *new_array =
            dynarr_grow_ut(array, capacity_increase, item_size, alignment);
        if (!new_array) {
            return NULL;
        }
        if ((uintptr_t)array != (uintptr_t)new_array) {
            // Arrays are different, so we need to replace the old one
            dynarr_header *new_header = dynarr_get_header(new_array);
            assert(new_header && "new header must not be null");
            dynarr_free(array);
            array = new_array;
            header = new_header;
        }
    }

    void *dest = ((u8 *)array) + header->len * item_size;
    bytes_copy(dest, items, count * item_size);
    header->len += count;
    return array;
}

b32 dynarr_pop_ut(void *array, void *out, size_t item_size) {
    if (!array) {
        return 0;
    }
    dynarr_header *header = dynarr_get_header(array);
    if (header->len == 0) {
        return 0;
    }
    u8 *item = ((u8 *)array) + (header->len - 1) * item_size;
    bytes_copy((u8 *)out, item, item_size);
    header->len -= 1;
    return 1;
}

b32 dynarr_remove_ut(void *array, u64 index, size_t item_size) {
    if (!array) {
        return 0;
    }
    dynarr_header *header = dynarr_get_header(array);
    if (header->len <= index) {
        return 0;
    }
    u8 *data = (u8 *)array;
    u8 *data_dst = data + index * item_size;
    u8 *data_src = data_dst + item_size;
    u64 bytes = (header->len - index - 1) * item_size;
    bytes_move(data_dst, data_src, bytes);
    header->len -= 1;
    return 1;
}

b32 dynarr_remove_uo_ut(void *array, u64 index, size_t item_size) {
    if (!array) {
        return 0;
    }
    dynarr_header *header = dynarr_get_header(array);
    if (header->len <= index) {
        return 0;
    }
    u8 *data = (u8 *)array;
    u8 *data_dst = data + index * item_size;
    u8 *data_src = data + (header->len - 1) * item_size;
    bytes_copy(data_dst, data_src, item_size);
    header->len -= 1;
    return 1;
}

////////////////////////
// C strings
////////////////////////

b32 cstr_eq_unsafe(const char *s1, const char *s2) {
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

b32 cstr_eq(const char *s1, const char *s2, size_t len) {
    if (s1 == s2) {
        return 1;
    }
    if (!s1 || !s2) {
        return 0;
    }

    for (size_t i = 0; i < len; i += 1) {
        if (s1[i] != s2[i]) {
            return 0;
        }
        if (!s1[i] && !s2[i]) {
            break;
        }
    }

    return 1;
}

size_t cstr_len_unsafe(const char *str) {
    if (!str) {
        return 0;
    }
    size_t len = 0;
    while (str[len]) { len += 1; }
    return len;
}

size_t cstr_len(const char *str, size_t capacity) {
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

slice cstr_split_next(cstr_split_iter *split) {
    assert(split && "split struct must not be null");
    assert(split->split_chars && "split chars must not be null");

    slice slice = {0};

    if (!split->str || !split->str_len || !split->split_chars_len
        || split->index >= split->str_len || !split->str[split->index]) {
        return slice;
    }

    slice.buffer = (u8 *)split->str + split->index;
    b32 len_set = 0;

    while (split->index < split->str_len && !len_set) {
        char ch = split->str[split->index];
        if (ch) {
            for (size_t si = 0; si < split->split_chars_len; si += 1) {
                if (split->split_chars[si] == ch) {
                    if (split->null_terminate) {
                        split->str[split->index] = '\0';
                    }
                    uintptr_t str = (uintptr_t)split->str;
                    uintptr_t buf = (uintptr_t)slice.buffer;
                    slice.len = (u64)(str + split->index - buf);
                    len_set = 1;
                    break;
                }
            }
        } else {
            uintptr_t str = (uintptr_t)split->str;
            uintptr_t buf = (uintptr_t)slice.buffer;
            slice.len = (u64)(str + split->index - buf);
            len_set = 1;
        }
        split->index += 1;
    }

    return slice;
}

size_t
cstr_split_collect(slice *arr, size_t len, cstr_split_iter *split) {
    assert(arr && "array must not be null");
    assert(split && "split must not be null");

    size_t i = 0;
    for (; i < len; i += 1) {
        slice slice = cstr_split_next(split);
        if (!slice.buffer) {
            break;
        }
        arr[i] = slice;
    }
    return i;
}

size_t cstr_split_collect_strings(
    char **strings, size_t len, cstr_split_iter *split
) {
    assert(strings && "strings must not be null");
    assert(split && "split must not be null");

    struct {
        u32 null_terminate : 1;
    } flags = {
        .null_terminate = split->null_terminate,
    };
    split->null_terminate = 1;

    size_t i = 0;
    for (; i < len; i += 1) {
        slice slice = cstr_split_next(split);
        if (!slice.buffer) {
            break;
        }
        strings[i] = (char *)slice.buffer;
    }

    split->null_terminate = flags.null_terminate;
    return i;
}

b32 cstr_match_wild_ascii(
    const char *txt, size_t txt_len, const char *pat, size_t pat_len
) {
    size_t i = 0, j = 0, start_index = 0, match = 0;
    b32 start_index_set = 0;

    while (i < txt_len) {
        if (j < pat_len && (pat[j] == '?' || pat[j] == txt[i])) {
            i += 1;
            j += 1;
        } else if (j < pat_len && pat[j] == '*') {
            start_index = j;
            start_index_set = 1;
            match = i;
            j += 1;
        } else if (start_index_set) {
            j = start_index + 1;
            match += 1;
            i = match;
        } else {
            return 0;
        }
    }

    while (j < pat_len && pat[j] == '*') { j += 1; }

    return j == pat_len;
}

b32 cstr_match_wild_ascii_unsafe(const char *txt, const char *pat) {
    size_t txt_len = cstr_len_unsafe(txt);
    size_t pat_len = cstr_len_unsafe(pat);
    return cstr_match_wild_ascii(txt, txt_len, pat, pat_len);
}

