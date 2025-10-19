#include "std.h"
#include <float.h>
#include <limits.h>

////////////////////////
// Bytes
////////////////////////

int bytes_diff_index(
    const void *a, const void *b, size_t start, size_t capacity
) {
    assert(start < capacity && "start must be lower than capacity");
    const uchar *a_ = a, *b_ = b;
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

bool bytes_eq(const void *a, const void *b, size_t capacity) {
    if (bytes_diff_index(a, b, 0, capacity) >= 0) {
        return 0;
    }
    return 1;
}

uchar byte_to_hex_char(uchar b) {
    assert(b < 16 && "byte must be between 0..=15");

    if (b < 10) {
        return (uchar)('0' + b);
    }
    return (uchar)('a' + b - 10);
}

void byte_to_hex_chars(uchar b, uchar *high, uchar *low) {
    *high = byte_to_hex_char(b / 16);
    *low = byte_to_hex_char(b % 16);
}

size_t bytes_to_hex(uchar *dest, const uchar *src, size_t n) {
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

slice slice_span(uchar *start, uchar *end) {
    assert(start && "start must not be null");
    assert(end && "end must not be null");

    uchar *temp;
    if ((uintptr_t)start > (uintptr_t)end) {
        temp = start;
        start = end;
        end = temp;
    }

    slice s = {0};
    s.buffer = start;
    s.len = (size_t)((uintptr_t)end - (uintptr_t)start);
    return s;
}

bool slice_eq(const slice a, const slice b) {
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
    slice.buffer = (uchar *)str;
    slice.len = cstr_len_unsafe(str);
    return slice;
}

////////////////////////
// Arena allocator
////////////////////////

arena arena_new(uchar *buffer, size_t size) {
    arena arena = {.buffer = buffer, .size = size, .used = 0};
    return arena;
}

void *arena_alloc_bytes(arena *arena, size_t size, size_t alignment) {
    size_t aligned_used = arena_aligned_used(arena->used, alignment);
    if (arena->size < size + aligned_used) {
        return NULL;
    }
    void *p = arena->buffer + aligned_used;
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
    allocator a = {.ctx = arena, .malloc = arena_malloc, .free = arena_free};
    return a;
}

////////////////////////
// Dynamic array
////////////////////////

void *dynarr_new_sized(
    ullong capacity, size_t item_size, size_t alignment, allocator *allocator
) {
    uchar *data = alloc_malloc(
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
    if (!header) {
        return;
    }
    alloc_free(header->allocator, header);
}

void *dynarr_grow_ut(
    void *array, ullong capacity_increase, size_t item_size, size_t alignment
) {
    if (!array) {
        return NULL;
    }
    dynarr_header *header = dynarr_get_header(array);
    assert(header && "Header must not be null");

    ullong capacity = capacity_increase + header->capacity;
    uchar *new_array_data = alloc_malloc(
        header->allocator, dynarr_count_to_bytes(capacity, item_size), alignment
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

    uchar *new_array = new_array_data + sizeof(dynarr_header);
    dynarr_header *new_header = (dynarr_header *)new_array_data;
    new_header->len = header->len;
    new_header->capacity = capacity;
    new_header->allocator = header->allocator;

    bytes_copy(new_array, array, new_header->len * item_size);
    return new_array;
}

void *dynarr_clone_ut(
    void *array, ullong capacity_increase, size_t item_size, size_t alignment
) {
    if (!array) {
        return NULL;
    }
    dynarr_header *header = dynarr_get_header(array);
    assert(header && "Header must not be null");

    ullong capacity = capacity_increase + header->capacity;
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

bool dynarr_push_ut(
    void *array, const void *items, ullong count, size_t item_size
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
        ((uchar *)array) + header->len * item_size, items, count * item_size
    );
    header->len += count;
    return 1;
}

void *dynarr_push_grow_ut(
    void *array,
    const void *items,
    ullong count,
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
        ullong capacity_increase = header->capacity + count + 8;
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

    void *dest = ((uchar *)array) + header->len * item_size;
    bytes_copy(dest, items, count * item_size);
    header->len += count;
    return array;
}

bool dynarr_pop_ut(void *array, void *out, size_t item_size) {
    if (!array) {
        return 0;
    }
    dynarr_header *header = dynarr_get_header(array);
    if (header->len == 0) {
        return 0;
    }
    uchar *item = ((uchar *)array) + (header->len - 1) * item_size;
    bytes_copy(out, item, item_size);
    header->len -= 1;
    return 1;
}

bool dynarr_remove_ut(void *array, ullong index, size_t item_size) {
    if (!array) {
        return 0;
    }
    dynarr_header *header = dynarr_get_header(array);
    if (header->len <= index) {
        return 0;
    }
    uchar *data = (uchar *)array;
    uchar *data_dst = data + index * item_size;
    uchar *data_src = data_dst + item_size;
    size_t bytes = (size_t)(header->len - index - 1) * item_size;
    bytes_move(data_dst, data_src, bytes);
    header->len -= 1;
    return 1;
}

bool dynarr_remove_uo_ut(void *array, ullong index, size_t item_size) {
    if (!array) {
        return 0;
    }
    dynarr_header *header = dynarr_get_header(array);
    if (header->len <= index) {
        return 0;
    }
    uchar *data = (uchar *)array;
    uchar *data_dst = data + index * item_size;
    uchar *data_src = data + (header->len - 1) * item_size;
    bytes_copy(data_dst, data_src, item_size);
    header->len -= 1;
    return 1;
}

////////////////////////
// Byte buffer
////////////////////////

bytebuf bytebuf_new(size_t capacity, allocator *allocator) {
    assert(allocator && "allocator must not be null");
    assert(capacity > 0 && "capacity must be larger than 0");
    bytebuf b;
    bytes_set(&b, 0, sizeof(b));
    b.allocator = allocator;
    b.buffer = alloc_malloc(allocator, capacity, _Alignof(uchar));
    if (b.buffer) {
        b.cap = capacity;
    }
    return b;
}

void bytebuf_free(bytebuf *bbuf) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(bbuf->allocator && "allocator must not be null");
    if (!bbuf && !bbuf->buffer) {
        return;
    }
    alloc_free(bbuf->allocator, bbuf->buffer);
    bbuf->cap = 0;
    bbuf->len = 0;
    bbuf->buffer = 0;
}

bool bytebuf_grow(bytebuf *bbuf, size_t capacity_increase) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(capacity_increase > 0 && "capacity increase must be larger than 0");
    if (capacity_increase == 0) {
        return 1;
    }

    uchar *newbuf = alloc_malloc(
        bbuf->allocator, bbuf->cap + capacity_increase, _Alignof(uchar)
    );
    if (!newbuf) {
        return 0;
    }

    bytes_copy(newbuf, bbuf->buffer, bbuf->len);
    bbuf->buffer = newbuf;
    bbuf->cap += capacity_increase;
    return 1;
}

bytebuf bytebuf_clone(bytebuf *bbuf, size_t capacity_increase) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(bbuf->allocator && "allocator must not be null");

    bytebuf newbbuf =
        bytebuf_new(bbuf->cap + capacity_increase, bbuf->allocator);
    if (!newbbuf.buffer) {
        return newbbuf; // failed new
    }
    bytes_copy(newbbuf.buffer, bbuf->buffer, bbuf->len);
    newbbuf.len = bbuf->len;
    return newbbuf;
}

bool bytebuf_write(bytebuf *bbuf, uchar *src, size_t len) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(src && "source must not be null");
    assert(len > 0 && "length must be more than 0");

    if (!src || len == 0) {
        return 1; // nothing to write
    }
    if (len > bbuf->cap - bbuf->len) {
        return 0; // no capacity left
    }
    bytes_copy(bbuf->buffer + bbuf->len, src, len);
    bbuf->len += len;
    return 1;
}

bool bytebuf_write_grow(bytebuf *bbuf, uchar *src, size_t len) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(src && "source must not be null");
    assert(len > 0 && "length must be more than 0");

    if (!src || len == 0) {
        return 1; // nothing to write
    }
    if (len > bbuf->cap - bbuf->len) {
        size_t capacity_increase = bbuf->cap + len;
        bool ok = bytebuf_grow(bbuf, capacity_increase);
        if (!ok) {
            return 0; // grow failed
        }
    }
    bytes_copy(bbuf->buffer + bbuf->len, src, len);
    bbuf->len += len;
    return 1;
}

void bytebuf_clear(bytebuf *bbuf) {
    bbuf->len = 0;
}

////////////////////////
// C strings
////////////////////////

bool cstr_eq_unsafe(const char *s1, const char *s2) {
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

bool cstr_eq(const char *s1, const char *s2, size_t len) {
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

    slice.buffer = (uchar *)split->str + split->index;
    bool len_set = 0;

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
                    slice.len = (size_t)(str + split->index - buf);
                    len_set = 1;
                    break;
                }
            }
        } else {
            uintptr_t str = (uintptr_t)split->str;
            uintptr_t buf = (uintptr_t)slice.buffer;
            slice.len = (size_t)(str + split->index - buf);
            len_set = 1;
        }
        split->index += 1;
    }

    return slice;
}

size_t cstr_split_collect(slice *arr, size_t len, cstr_split_iter *split) {
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

size_t
cstr_split_collect_strings(char **strings, size_t len, cstr_split_iter *split) {
    assert(strings && "strings must not be null");
    assert(split && "split must not be null");

    struct {
        uint null_terminate : 1;
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

bool cstr_match_wild_ascii(
    const char *txt, size_t txt_len, const char *pat, size_t pat_len
) {
    size_t i = 0, j = 0, start_index = 0, match = 0;
    bool start_index_set = 0;

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

bool cstr_match_wild_ascii_unsafe(const char *txt, const char *pat) {
    size_t txt_len = cstr_len_unsafe(txt);
    size_t pat_len = cstr_len_unsafe(pat);
    return cstr_match_wild_ascii(txt, txt_len, pat, pat_len);
}

bool cstr_to_int(const char *s, size_t len, int *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    int v_ = 0;
    size_t i = 0;
    int sign = 1;

    if (s[0] == '-') {
        i += 1;
        sign = -1;
    }

    for (; i < len && s[i]; i += 1) {
        if (s[i] >= '0' && s[i] <= '9') {
            int n = s[i] - '0';
            if ((INT_MAX - n) / 10 < v_) {
                // exceeds int size
                return 0;
            }
            v_ *= 10;
            v_ += n;
        } else {
            // invalid character
            return 0;
        }
    }

    *v = sign * v_;
    return 1;
}

bool cstr_to_uint(const char *s, size_t len, uint *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    uint v_ = 0;

    for (size_t i = 0; i < len && s[i]; i += 1) {
        if (s[i] >= '0' && s[i] <= '9') {
            uint n = (uchar)s[i] - '0';
            if ((UINT_MAX - n) / 10 < v_) {
                // exceeds int size
                return 0;
            }
            v_ *= 10;
            v_ += n;
        } else {
            // invalid character
            return 0;
        }
    }

    *v = v_;
    return 1;
}

bool cstr_to_llong(const char *s, size_t len, llong *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    llong v_ = 0;
    size_t i = 0;
    llong sign = 1;

    if (s[0] == '-') {
        i += 1;
        sign = -1;
    }

    for (; i < len && s[i]; i += 1) {
        if (s[i] >= '0' && s[i] <= '9') {
            llong n = s[i] - '0';
            if ((LLONG_MAX - n) / 10 < v_) {
                // exceeds int size
                return 0;
            }
            v_ *= 10;
            v_ += n;
        } else {
            // invalid character
            return 0;
        }
    }

    *v = sign * v_;
    return 1;
}

bool cstr_to_ullong(const char *s, size_t len, ullong *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    ullong v_ = 0;

    for (size_t i = 0; i < len && s[i]; i += 1) {
        if (s[i] >= '0' && s[i] <= '9') {
            ullong n = (uchar)s[i] - '0';
            if ((ULLONG_MAX - n) / 10 < v_) {
                // exceeds int size
                return 0;
            }
            v_ *= 10;
            v_ += n;
        } else {
            // invalid character
            return 0;
        }
    }

    *v = v_;
    return 1;
}

static const double pow10d[] = {
    1e0,   1e1,   1e2,   1e3,   1e4,   1e5,   1e6,   1e7,   1e8,   1e9,   1e10,
    1e11,  1e12,  1e13,  1e14,  1e15,  1e16,  1e17,  1e18,  1e19,  1e20,  1e21,
    1e22,  1e23,  1e24,  1e25,  1e26,  1e27,  1e28,  1e29,  1e30,  1e31,  1e32,
    1e33,  1e34,  1e35,  1e36,  1e37,  1e38,  1e39,  1e40,  1e41,  1e42,  1e43,
    1e44,  1e45,  1e46,  1e47,  1e48,  1e49,  1e50,  1e51,  1e52,  1e53,  1e54,
    1e55,  1e56,  1e57,  1e58,  1e59,  1e60,  1e61,  1e62,  1e63,  1e64,  1e65,
    1e66,  1e67,  1e68,  1e69,  1e70,  1e71,  1e72,  1e73,  1e74,  1e75,  1e76,
    1e77,  1e78,  1e79,  1e80,  1e81,  1e82,  1e83,  1e84,  1e85,  1e86,  1e87,
    1e88,  1e89,  1e90,  1e91,  1e92,  1e93,  1e94,  1e95,  1e96,  1e97,  1e98,
    1e99,  1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
    1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119, 1e120,
    1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129, 1e130, 1e131,
    1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139, 1e140, 1e141, 1e142,
    1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149, 1e150, 1e151, 1e152, 1e153,
    1e154, 1e155, 1e156, 1e157, 1e158, 1e159, 1e160, 1e161, 1e162, 1e163, 1e164,
    1e165, 1e166, 1e167, 1e168, 1e169, 1e170, 1e171, 1e172, 1e173, 1e174, 1e175,
    1e176, 1e177, 1e178, 1e179, 1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186,
    1e187, 1e188, 1e189, 1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197,
    1e198, 1e199, 1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208,
    1e209, 1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
    1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229, 1e230,
    1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239, 1e240, 1e241,
    1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249, 1e250, 1e251, 1e252,
    1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259, 1e260, 1e261, 1e262, 1e263,
    1e264, 1e265, 1e266, 1e267, 1e268, 1e269, 1e270, 1e271, 1e272, 1e273, 1e274,
    1e275, 1e276, 1e277, 1e278, 1e279, 1e280, 1e281, 1e282, 1e283, 1e284, 1e285,
    1e286, 1e287, 1e288, 1e289, 1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296,
    1e297, 1e298, 1e299, 1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307,
    1e308
};

static const ullong pow10ullong[] = {
    1e0L,
    1e1L,
    1e2L,
    1e3L,
    1e4L,
    1e5L,
    1e6L,
    1e7L,
    1e8L,
    1e9L,
    1e10L,
    1e11L,
    1e12L,
    1e13L,
    1e14L,
    1e15L,
    1e16L,
    1e17L,
    1e18L
};

bool cstr_to_float(const char *s, size_t len, float *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    ullong integer = 0;
    ullong decimals = 0;
    float fraction = 0.0;
    float sign = 1.0;
    ullong exp = 0;
    size_t i = 0;

    if (s[0] == '-') {
        i += 1;
        sign = -1.0;
    } else if (s[0] == '+') {
        i += 1;
    }

    // integer part
    for (; i < len && s[i] >= '0' && s[i] <= '9'; i += 1) {
        ullong n = (uchar)s[i] - '0';
        if ((INT64_MAX - n) / 10 < integer) {
            // exceeds int size
            return 0;
        }
        integer *= 10;
        integer += n;
    }

    // decimals
    if (s[i] == '.') {
        i += 1;
        float divisor = 10.0;
        for (; i < len && s[i] >= '0' && s[i] <= '9'; i += 1) {
            ullong n = (uchar)s[i] - '0';
            if ((INT64_MAX - n) / 10 < integer) {
                // exceeds int size, use fractions instead
                fraction += (float)(s[i] - '0') / divisor;
                divisor *= 10.0f;
                return 0;
            } else {
                integer *= 10;
                integer += n;
                decimals += 1;
            }
        }
    }

    // exponent
    if (s[i] == 'e' || s[i] == 'E') {
        i += 1;

        int exp_sign = 1;
        if (s[i] == '-') {
            exp_sign = -1;
            i += 1;
        } else if (s[i] == '+') {
            i += 1;
        }

        for (; i < len && s[i] >= '0' && s[i] <= '9'; i += 1) {
            ullong n = (uchar)s[i] - '0';
            if ((INT64_MAX - n) / 10 < exp) {
                // exceeds int size
                return 0;
            }
            exp *= 10;
            exp += n;
        }

        assert(exp_sign != 0 && "exp_sign must not be zero");
        if (exp_sign > 0) {
            ullong decimals_to_reduce = min(decimals, exp);
            decimals -= decimals_to_reduce;
            exp -= decimals_to_reduce;
        } else if (exp_sign < 0) {
            if ((INT64_MAX - exp < decimals)) {
                // exceeds int size
                return 0;
            }
            decimals += exp;
            exp = 0;
        }

        if (exp > FLT_MAX_10_EXP) {
            // exceeds max exponent
            return 0;
        }
    }

    if (i < len && s[i] != '\0') {
        // failed to parse all the way to the end
        return 0;
    }

    // collect results
    float v_ = (float)integer;
    if (v_ > FLT_MAX / (float)pow10d[exp]) {
        // exceeds float max
        return 0;
    }
    v_ *= (float)pow10d[exp];
    v_ /= (float)pow10d[decimals];
    if (v_ > FLT_MAX - fraction) {
        // exceeds float max
        return 0;
    }
    v_ += fraction;
    v_ *= sign;

    *v = v_;
    return 1;
}

bool cstr_to_double(const char *s, size_t len, double *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    ullong integer = 0;
    ullong decimals = 0;
    double fraction = 0.0;
    float sign = 1.0;
    ullong exp = 0;
    size_t i = 0;

    if (s[0] == '-') {
        i += 1;
        sign = -1.0;
    } else if (s[0] == '+') {
        i += 1;
    }

    // integer part
    for (; i < len && s[i] >= '0' && s[i] <= '9'; i += 1) {
        ullong n = (uchar)s[i] - '0';
        if ((INT64_MAX - n) / 10 < integer) {
            // exceeds int size
            return 0;
        }
        integer *= 10;
        integer += n;
    }

    // decimals
    if (s[i] == '.') {
        i += 1;
        double divisor = 10.0;
        for (; i < len && s[i] >= '0' && s[i] <= '9'; i += 1) {
            ullong n = (uchar)s[i] - '0';
            if ((INT64_MAX - n) / 10 < integer) {
                // exceeds int size, use fractions instead
                fraction += (double)(s[i] - '0') / divisor;
                divisor *= 10.0;
                return 0;
            } else {
                integer *= 10;
                integer += n;
                decimals += 1;
            }
        }
    }

    // exponent
    if (s[i] == 'e' || s[i] == 'E') {
        i += 1;

        int exp_sign = 1;
        if (s[i] == '-') {
            exp_sign = -1;
            i += 1;
        } else if (s[i] == '+') {
            i += 1;
        }

        for (; i < len && s[i] >= '0' && s[i] <= '9'; i += 1) {
            ullong n = (uchar)s[i] - '0';
            if ((INT64_MAX - n) / 10 < exp) {
                // exceeds int size
                return 0;
            }
            exp *= 10;
            exp += n;
        }

        assert(exp_sign != 0 && "exp_sign must not be zero");
        if (exp_sign > 0) {
            ullong decimals_to_reduce = min(decimals, exp);
            decimals -= decimals_to_reduce;
            exp -= decimals_to_reduce;
        } else if (exp_sign < 0) {
            if ((INT64_MAX - exp < decimals)) {
                // exceeds int size
                return 0;
            }
            decimals += exp;
            exp = 0;
        }

        if (exp > DBL_MAX_10_EXP) {
            // exceeds max exponent
            return 0;
        }
    }

    if (i < len && s[i] != '\0') {
        // failed to parse all the way to the end
        return 0;
    }

    // collect results
    double v_ = (double)integer;
    if (v_ > DBL_MAX / pow10d[exp]) {
        // exceeds double max
        return 0;
    }
    v_ *= pow10d[exp];
    v_ /= pow10d[decimals];
    if (v_ > DBL_MAX - fraction) {
        // exceeds double max
        return 0;
    }
    v_ += fraction;
    v_ *= sign;

    *v = v_;
    return 1;
}

size_t cstr_from_int(char *dest, size_t len, int src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");
    char tmp[16] = {0};
    char *end = tmp + sizeof(tmp);
    char *cursor = end - 1;
    bool is_neg = src < 0;
    if (is_neg) {
        src = -src;
    }

    do {
        cursor -= 1;
        *cursor = '0' + (char)(src % 10);
        src /= 10;
    } while (src > 0);
    if (is_neg) {
        cursor -= 1;
        *cursor = '-';
    }

    size_t tmp_len = (size_t)((uintptr_t)end - (uintptr_t)cursor);
    size_t bytes_to_copy = min(len, tmp_len);
    bytes_copy(dest, cursor, bytes_to_copy);
    return bytes_to_copy;
}

size_t cstr_from_uint(char *dest, size_t len, uint src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");
    char tmp[16] = {0};
    char *end = tmp + sizeof(tmp);
    char *cursor = end - 1;

    do {
        cursor -= 1;
        *cursor = '0' + (char)(src % 10);
        src /= 10;
    } while (src > 0);

    size_t tmp_len = (size_t)((uintptr_t)end - (uintptr_t)cursor);
    size_t bytes_to_copy = min(len, tmp_len);
    bytes_copy(dest, cursor, bytes_to_copy);
    return bytes_to_copy;
}

size_t cstr_from_llong(char *dest, size_t len, llong src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");
    char tmp[32] = {0};
    char *end = tmp + sizeof(tmp);
    char *cursor = end - 1;
    bool is_neg = src < 0;
    if (is_neg) {
        src = -src;
    }

    do {
        cursor -= 1;
        *cursor = '0' + (char)(src % 10);
        src /= 10;
    } while (src > 0);
    if (is_neg) {
        cursor -= 1;
        *cursor = '-';
    }

    size_t tmp_len = (size_t)((uintptr_t)end - (uintptr_t)cursor);
    size_t bytes_to_copy = min(len, tmp_len);
    bytes_copy(dest, cursor, bytes_to_copy);
    return bytes_to_copy;
}

size_t cstr_from_ullong(char *dest, size_t len, ullong src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");
    char tmp[32] = {0};
    char *end = tmp + sizeof(tmp);
    char *cursor = end - 1;

    do {
        cursor -= 1;
        *cursor = '0' + (char)(src % 10);
        src /= 10;
    } while (src > 0);

    size_t tmp_len = (size_t)((uintptr_t)end - (uintptr_t)cursor);
    size_t bytes_to_copy = min(len, tmp_len);
    bytes_copy(dest, cursor, bytes_to_copy);
    return bytes_to_copy;
}

size_t cstr_from_float(char *dest, size_t len, float src, uint decimals) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");
    assert(decimals < 19 && "decimals up to 18 are supported");
    if (len == 0) {
        return 0;
    }

    size_t bytes_written = 0;
    if (src < 0) {
        dest[bytes_written] = '-';
        bytes_written += 1;
        src = -src;
    }
    if (bytes_written == len) {
        return bytes_written;
    }

    double precision_d = pow10d[decimals];

    src += 0.5f / (float)precision_d;
    if (src >= (float)(-1UL >> 1)) {
        // necessary?
        size_t bytes_to_copy = min(len, sizeof("inf"));
        bytes_copy(dest + bytes_written, "inf", bytes_to_copy);
        bytes_written += bytes_to_copy;
        return bytes_written;
    }

    // integer part
    ullong integer = (ullong)src;
    bytes_written +=
        cstr_from_ullong(dest + bytes_written, len - bytes_written, integer)
        - 1;
    if (bytes_written == len) {
        return bytes_written;
    }

    // decimal point
    dest[bytes_written] = '.';
    bytes_written += 1;
    if (bytes_written == len) {
        return bytes_written;
    }

    // fractional part
    ullong fractional = (ullong)((src - (double)integer) * precision_d);
    ullong precision_ullong = pow10ullong[decimals];
    for (ullong i = precision_ullong / 10; i > 1; i /= 10) {
        if (i > fractional) {
            dest[bytes_written] = '0';
            bytes_written += 1;
            if (bytes_written == len) {
                return bytes_written;
            }
        }
    }
    bytes_written +=
        cstr_from_ullong(dest + bytes_written, len - bytes_written, fractional);

    return bytes_written;
}

size_t cstr_from_double(char *dest, size_t len, double src, uint decimals) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");
    assert(decimals < 19 && "decimals up to 19 are supported");
    if (len == 0) {
        return 0;
    }

    size_t bytes_written = 0;
    if (src < 0) {
        dest[bytes_written] = '-';
        bytes_written += 1;
        src = -src;
    }
    if (bytes_written == len) {
        return bytes_written;
    }

    double precision_d = pow10d[decimals];

    src += 0.5 / precision_d;
    if (src >= (double)(-1UL >> 1)) {
        // necessary?
        size_t bytes_to_copy = min(len, sizeof("inf"));
        bytes_copy(dest + bytes_written, "inf", bytes_to_copy);
        bytes_written += bytes_to_copy;
        return bytes_written;
    }

    // integer part
    ullong integer = (ullong)src;
    bytes_written +=
        cstr_from_ullong(dest + bytes_written, len - bytes_written, integer)
        - 1;
    if (bytes_written == len) {
        return bytes_written;
    }

    // decimal point
    dest[bytes_written] = '.';
    bytes_written += 1;
    if (bytes_written == len) {
        return bytes_written;
    }

    // fractional part
    ullong fractional = (ullong)((src - (double)integer) * precision_d);
    ullong precision_ullong = pow10ullong[decimals];
    for (ullong i = precision_ullong / 10; i > 1; i /= 10) {
        if (i > fractional) {
            dest[bytes_written] = '0';
            bytes_written += 1;
            if (bytes_written == len) {
                return bytes_written;
            }
        }
    }
    bytes_written +=
        cstr_from_ullong(dest + bytes_written, len - bytes_written, fractional);

    return bytes_written;
}
