#include "std.h"
#include <float.h>
#include <limits.h>
#include <stdarg.h>

////////////////////////
// Bytes
////////////////////////

int bytes_diff_index(const void *a, const void *b, size_t start, size_t len) {
    assert(start < len && "start must be lower than length");
    const uchar *a_ = a, *b_ = b;
    if ((uintptr_t)a_ == (uintptr_t)b_) {
        return -1;
    }
    if (!a || !b) {
        return 0;
    }

    for (size_t i = start; i < len; i += 1) {
        if (a_[i] != b_[i]) {
            return (int)i;
        }
    }

    return -1;
}

bool bytes_eq(const void *a, const void *b, size_t len) {
    if (bytes_diff_index(a, b, 0, len) >= 0) {
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
// Allocator
////////////////////////

struct mmap_alloc_header {
    size_t size;
};

void *mmap_malloc(size_t size, size_t alignment, void *ctx) {
    assert(size > 0 && "size must be greater than 0");
    (void)alignment;
    (void)ctx;

    long page_size_signed = sysconf(_SC_PAGE_SIZE);
    assert(page_size_signed > 0 && "expected a page size >0");

    size_t header_alloc_size = sizeof(struct mmap_alloc_header);
    size += header_alloc_size;
    size = (size_t)round_up_multiple_ullong(
        (ullong)size, (ullong)page_size_signed
    );

    void *p = mmap(
        0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );
    if (p == MAP_FAILED) {
        return NULL;
    }

    struct mmap_alloc_header *header = p;
    header->size = size;

    return (uchar *)p + header_alloc_size;
}

void mmap_free(void *ptr, void *ctx) {
    assert(ptr && "ptr must not be null");
    (void)ctx;

    struct mmap_alloc_header *header =
        (struct mmap_alloc_header *)((uchar *)ptr
                                     - sizeof(struct mmap_alloc_header));
    munmap(header, header->size);
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

bool slice_const_eq(const slice_const a, const slice_const b) {
    if (a.len != b.len) {
        return 0;
    }
    return bytes_eq(a.buffer, b.buffer, a.len);
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

slice_const slice_const_from_cstr_unsafe(const char *str) {
    slice_const slice = {0};
    slice.buffer = (const uchar *)str;
    slice.len = cstr_len_unsafe(str);
    return slice;
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
    size_t aligned_used = align_to_nearest(arena->used, alignment);
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
    alignment = max(alignment, alignof(dynarr_header));
    void *data = alloc_malloc(
        allocator, dynarr_count_to_bytes(capacity, item_size), alignment
    );
    if (!data) {
        return NULL;
    }

    dynarr_header *header = (dynarr_header *)data;
    header->len = 0;
    header->capacity = capacity;
    header->allocator = allocator;

    return (uchar *)(data) + sizeof(dynarr_header);
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
    alignment = max(alignment, alignof(dynarr_header));
    void *new_array_data = alloc_malloc(
        header->allocator, dynarr_count_to_bytes(capacity, item_size), alignment
    );
    if (!new_array_data) {
        return NULL;
    }

    uchar *new_array = (uchar *)(new_array_data) + sizeof(dynarr_header);
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

bool char_is_digit(char c) {
    return c >= '0' && c <= '9';
}

size_t cstr_to_int(const char *s, size_t len, int *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    size_t bytes_parsed = 0;
    size_t i = 0;
    int v_ = 0;
    int sign = 1;

    if (s[0] == '-') {
        i += 1;
        sign = -1;
        bytes_parsed += 1;
    }

    for (; i < len && char_is_digit(s[i]); i += 1) {
        int n = s[i] - '0';
        if ((INT_MAX - n) / 10 < v_) {
            // exceeds int size
            return 0;
        }
        v_ *= 10;
        v_ += n;
        bytes_parsed += 1;
    }

    *v = sign * v_;
    return bytes_parsed;
}

size_t cstr_to_uint(const char *s, size_t len, uint *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    size_t bytes_parsed = 0;
    uint v_ = 0;

    for (size_t i = 0; i < len && char_is_digit(s[i]); i += 1) {
        uint n = (uchar)s[i] - '0';
        if ((UINT_MAX - n) / 10 < v_) {
            // exceeds int size
            return 0;
        }
        v_ *= 10;
        v_ += n;
        bytes_parsed += 1;
    }

    *v = v_;
    return bytes_parsed;
}

size_t cstr_to_llong(const char *s, size_t len, llong *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    size_t bytes_parsed = 0;
    size_t i = 0;
    llong v_ = 0;
    llong sign = 1;

    if (s[0] == '-') {
        i += 1;
        sign = -1;
        bytes_parsed += 1;
    }

    for (; i < len && char_is_digit(s[i]); i += 1) {
        llong n = s[i] - '0';
        if ((LLONG_MAX - n) / 10 < v_) {
            // exceeds int size
            return 0;
        }
        v_ *= 10;
        v_ += n;
        bytes_parsed += 1;
    }

    *v = sign * v_;
    return bytes_parsed;
}

size_t cstr_to_ullong(const char *s, size_t len, ullong *v) {
    assert(s && "string must not be null");
    assert(len && "length must be 1 or more");
    assert(v && "value storage must not be null");
    if (!s || !len || !v) {
        return 0;
    }
    size_t bytes_parsed = 0;
    ullong v_ = 0;

    for (size_t i = 0; i < len && char_is_digit(s[i]); i += 1) {
        ullong n = (uchar)s[i] - '0';
        if ((ULLONG_MAX - n) / 10 < v_) {
            // exceeds int size
            return 0;
        }
        v_ *= 10;
        v_ += n;
        bytes_parsed += 1;
    }

    *v = v_;
    return bytes_parsed;
}

static const double pow10_double[] = {
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

static const ullong pow10_ullong[] = {
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

size_t cstr_to_float(const char *s, size_t len, float *v) {
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

    if (i < len && s[0] == '-') {
        i += 1;
        sign = -1.0;
    } else if (i < len && s[0] == '+') {
        i += 1;
    }

    // integer part
    for (; i < len && char_is_digit(s[i]); i += 1) {
        ullong n = (uchar)s[i] - '0';
        if ((INT64_MAX - n) / 10 < integer) {
            // exceeds int size
            return 0;
        }
        integer *= 10;
        integer += n;
    }

    // decimals
    if (i < len && s[i] == '.') {
        i += 1;
        float divisor = 10.0;
        for (; i < len && char_is_digit(s[i]); i += 1) {
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
    if (i < len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;

        int exp_sign = 1;
        if (i < len && s[i] == '-') {
            exp_sign = -1;
            i += 1;
        } else if (i < len && s[i] == '+') {
            i += 1;
        }

        for (; i < len && char_is_digit(s[i]); i += 1) {
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

    // collect results
    float v_ = (float)integer;
    if (v_ > FLT_MAX / (float)pow10_double[exp]) {
        // exceeds float max
        return 0;
    }
    v_ *= (float)pow10_double[exp];
    v_ /= (float)pow10_double[decimals];
    if (v_ > FLT_MAX - fraction) {
        // exceeds float max
        return 0;
    }
    v_ += fraction;
    v_ *= sign;

    *v = v_;
    return i;
}

size_t cstr_to_double(const char *s, size_t len, double *v) {
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

    if (i < len && s[0] == '-') {
        i += 1;
        sign = -1.0;
    } else if (i < len && s[0] == '+') {
        i += 1;
    }

    // integer part
    for (; i < len && char_is_digit(s[i]); i += 1) {
        ullong n = (uchar)s[i] - '0';
        if ((INT64_MAX - n) / 10 < integer) {
            // exceeds int size
            return 0;
        }
        integer *= 10;
        integer += n;
    }

    // decimals
    if (i < len && s[i] == '.') {
        i += 1;
        double divisor = 10.0;
        for (; i < len && char_is_digit(s[i]); i += 1) {
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
    if (i < len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;

        int exp_sign = 1;
        if (i < len && s[i] == '-') {
            exp_sign = -1;
            i += 1;
        } else if (i < len && s[i] == '+') {
            i += 1;
        }

        for (; i < len && char_is_digit(s[i]); i += 1) {
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

    // collect results
    double v_ = (double)integer;
    if (v_ > DBL_MAX / pow10_double[exp]) {
        // exceeds double max
        return 0;
    }
    v_ *= pow10_double[exp];
    v_ /= pow10_double[decimals];
    if (v_ > DBL_MAX - fraction) {
        // exceeds double max
        return 0;
    }
    v_ += fraction;
    v_ *= sign;

    *v = v_;
    return i;
}

#define cstr_int_max_len 16
#define cstr_uint_max_len 16
#define cstr_llong_max_len 32
#define cstr_ullong_max_len 32

static size_t cstr_from_int_unsafe(char *cursor, int src) {
    char *start = cursor;
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

    return (size_t)(start - cursor);
}

size_t cstr_from_int(char *dest, size_t len, int src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");

    char tmp[cstr_int_max_len];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_int_unsafe(end, src);

    if (len < bytes_to_copy) {
        return 0;
    }

    bytes_copy(dest, end - bytes_to_copy, bytes_to_copy);
    return bytes_to_copy;
}

static size_t cstr_from_uint_unsafe(char *cursor, uint src) {
    char *start = cursor;

    do {
        cursor -= 1;
        *cursor = '0' + (char)(src % 10);
        src /= 10;
    } while (src > 0);

    return (size_t)(start - cursor);
}

size_t cstr_from_uint(char *dest, size_t len, uint src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");

    char tmp[cstr_uint_max_len];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_uint_unsafe(end, src);

    if (len < bytes_to_copy) {
        return 0;
    }

    bytes_copy(dest, end - bytes_to_copy, bytes_to_copy);
    return bytes_to_copy;
}

static size_t cstr_from_llong_unsafe(char *cursor, llong src) {
    char *start = cursor;
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

    return (size_t)(start - cursor);
}

size_t cstr_from_llong(char *dest, size_t len, llong src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");

    char tmp[cstr_llong_max_len];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_llong_unsafe(end, src);

    if (len < bytes_to_copy) {
        return 0;
    }

    bytes_copy(dest, end - bytes_to_copy, bytes_to_copy);
    return bytes_to_copy;
}

static size_t cstr_from_ullong_unsafe(char *cursor, ullong src) {
    char *start = cursor;

    do {
        cursor -= 1;
        *cursor = '0' + (char)(src % 10);
        src /= 10;
    } while (src > 0);

    return (size_t)(start - cursor);
}

size_t cstr_from_ullong(char *dest, size_t len, ullong src) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be more than 0");

    char tmp[cstr_ullong_max_len];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_ullong_unsafe(end, src);

    if (len < bytes_to_copy) {
        return 0;
    }

    bytes_copy(dest, end - bytes_to_copy, bytes_to_copy);
    return bytes_to_copy;
}

struct cstr_from_real_parts {
    char *integer_cursor;
    char *fractional_cursor;
    size_t integer_len;
    size_t fractional_len;
    uint decimal_zeros;
    bool is_neg;
    bool is_inf;
};

static size_t cstr_from_real_parts_len(struct cstr_from_real_parts *parts) {
    size_t neg_len = parts->is_neg ? 1UL : 0UL;
    if (parts->is_inf) {
        return neg_len + 3; // len('inf') == 3
    }

    return neg_len + parts->integer_len + 1 + parts->decimal_zeros
        + parts->fractional_len;
}

static void
cstr_from_real_parts_to_buf(struct cstr_from_real_parts *parts, char *dest) {
    if (parts->is_inf) {
        const char *text = "inf";
        size_t bytes_to_write = sizeof("inf") - 1;
        if (parts->is_neg) {
            text = "-inf";
            bytes_to_write = sizeof("-inf") - 1;
        }
        bytes_copy(dest, text, bytes_to_write);
        return;
    }

    char *cursor = dest;
    if (parts->is_neg) {
        cursor[0] = '-';
        cursor += 1;
    }
    bytes_copy(
        cursor, parts->integer_cursor - parts->integer_len, parts->integer_len
    );
    cursor += parts->integer_len;
    cursor[0] = '.';
    cursor += 1;
    for (uint i = 0; i < parts->decimal_zeros; i += 1) { cursor[i] = '0'; }
    cursor += parts->decimal_zeros;
    bytes_copy(
        cursor,
        parts->fractional_cursor - parts->fractional_len,
        parts->fractional_len
    );
    cursor += parts->fractional_len;

    assert((size_t)(cursor - dest) == cstr_from_real_parts_len(parts));
    return;
}

static uint count_decimal_zeros(ullong fractional, uint decimals) {
    ullong precision_ullong = pow10_ullong[decimals];
    uint decimal_zeros = 0;
    for (ullong i = precision_ullong / 10; i > 1; i /= 10) {
        if (i > fractional) {
            decimal_zeros += 1;
        } else {
            break;
        }
    }
    return decimal_zeros;
}

static void cstr_from_float_parts(
    struct cstr_from_real_parts *parts, float src, uint decimals
) {
    assert(parts->integer_cursor && "integer cursor must not be null");
    assert(parts->fractional_cursor && "integer cursor must not be null");
    assert(decimals <= 19 && "decimals up to 18 are supported");

    if (decimals > 19) {
        return;
    }

    if (src < 0) {
        parts->is_neg = 1;
        src = -src;
    }

    double precision_d = pow10_double[decimals];
    src += (float)(0.5 / precision_d);

    // infinity check
    if (src >= (float)(-1UL >> 1)) {
        parts->is_inf = 1;
        return;
    }

    // integer part
    ullong integer = (ullong)src;
    parts->integer_len =
        cstr_from_ullong_unsafe(parts->integer_cursor, integer);

    // fractional part
    ullong fractional = (ullong)(((double)src - (double)integer) * precision_d);
    parts->decimal_zeros = count_decimal_zeros(fractional, decimals);
    parts->fractional_len =
        cstr_from_ullong_unsafe(parts->fractional_cursor, fractional);
}

size_t cstr_from_float(char *dest, size_t len, float src, uint decimals) {
    assert(dest && "dest must not be null");
    assert(len > 2 && "len must be more than 2");

    if (len < 3) {
        return 0;
    }

    char integer_cstr[cstr_ullong_max_len];
    char fractional_cstr[cstr_ullong_max_len];
    struct cstr_from_real_parts parts = {0};
    parts.integer_cursor = integer_cstr + sizeof(integer_cstr);
    parts.fractional_cursor = fractional_cstr + sizeof(fractional_cstr);
    cstr_from_float_parts(&parts, src, decimals);
    size_t bytes_to_write = cstr_from_real_parts_len(&parts);

    if (bytes_to_write > len) {
        return 0;
    }

    cstr_from_real_parts_to_buf(&parts, dest);
    return bytes_to_write;
}

static void cstr_from_double_parts(
    struct cstr_from_real_parts *parts, double src, uint decimals
) {
    assert(parts->integer_cursor && "integer cursor must not be null");
    assert(parts->fractional_cursor && "integer cursor must not be null");
    assert(decimals <= 19 && "decimals up to 18 are supported");

    if (decimals > 19) {
        return;
    }

    if (src < 0) {
        parts->is_neg = 1;
        src = -src;
    }

    double precision_d = pow10_double[decimals];
    src += 0.5 / precision_d;

    // infinity check
    if (src >= (double)(-1UL >> 1)) {
        parts->is_inf = 1;
        return;
    }

    // integer part
    ullong integer = (ullong)src;
    parts->integer_len =
        cstr_from_ullong_unsafe(parts->integer_cursor, integer);

    // fractional part
    ullong fractional = (ullong)((src - (double)integer) * precision_d);
    parts->decimal_zeros = count_decimal_zeros(fractional, decimals);
    parts->fractional_len =
        cstr_from_ullong_unsafe(parts->fractional_cursor, fractional);
}

size_t cstr_from_double(char *dest, size_t len, double src, uint decimals) {
    assert(dest && "dest must not be null");
    assert(len > 2 && "len must be more than 2");

    if (len < 3) {
        return 0;
    }

    char integer_cstr[cstr_ullong_max_len];
    char fractional_cstr[cstr_ullong_max_len];
    struct cstr_from_real_parts parts = {0};
    parts.integer_cursor = integer_cstr + sizeof(integer_cstr);
    parts.fractional_cursor = fractional_cstr + sizeof(fractional_cstr);
    cstr_from_double_parts(&parts, src, decimals);
    size_t bytes_to_write = cstr_from_real_parts_len(&parts);

    if (bytes_to_write > len) {
        return 0;
    }

    cstr_from_real_parts_to_buf(&parts, dest);
    return bytes_to_write;
}

size_t cstr_len_int(int src) {
    size_t len = 0;
    if (src < 0) {
        len += 1;
        src = -src;
    }
    do {
        len += 1;
        src /= 10;
    } while (src > 0);
    return len;
}

size_t cstr_len_uint(uint src) {
    size_t len = 0;
    do {
        len += 1;
        src /= 10;
    } while (src > 0);
    return len;
}

size_t cstr_len_llong(llong src) {
    size_t len = 0;
    if (src < 0) {
        len += 1;
        src = -src;
    }
    do {
        len += 1;
        src /= 10;
    } while (src > 0);
    return len;
}

size_t cstr_len_ullong(ullong src) {
    size_t len = 0;
    do {
        len += 1;
        src /= 10;
    } while (src > 0);
    return len;
}

size_t cstr_len_float(float src, uint decimals) {
    assert(decimals < 19 && "decimals up to 19 are supported");

    size_t len = 0;
    if (src < 0) {
        len += 1;
        src = -src;
    }

    double precision_d = pow10_double[decimals];

    src += 0.5f / (float)precision_d;
    if (src >= (float)(-1UL >> 1)) {
        return len + sizeof("inf") - 1;
    }

    // integer part
    ullong integer = (ullong)src;
    len += cstr_len_ullong(integer);

    // decimal point
    len += 1;

    // fractional part
    ullong fractional = (ullong)((src - (double)integer) * precision_d);
    len += count_decimal_zeros(fractional, decimals);
    len += cstr_len_ullong(fractional);

    return len;
}

size_t cstr_len_double(double src, uint decimals) {
    assert(decimals < 19 && "decimals up to 19 are supported");

    size_t len = 0;
    if (src < 0) {
        len += 1;
        src = -src;
    }

    double precision_d = pow10_double[decimals];

    src += 0.5 / precision_d;
    if (src >= (double)(-1UL >> 1)) {
        return len + sizeof("inf") - 1;
    }

    // integer part
    ullong integer = (ullong)src;
    len += cstr_len_ullong(integer);

    // decimal point
    len += 1;

    // fractional part
    ullong fractional = (ullong)((src - (double)integer) * precision_d);
    len += count_decimal_zeros(fractional, decimals);
    len += cstr_len_ullong(fractional);

    return len;
}

cstr_fmt_result cstr_fmt_va(
    char *restrict dest,
    size_t len,
    const char *restrict format,
    va_list va_args
) {
    assert(dest && "dest must not be null");
    assert(len > 0 && "len must be higher than 0");
    assert(format && "format must not be null");

    cstr_fmt_result res = {
        .ok = 1,
        .len = 0,
    };
    size_t bytes_written = 0;

    if (!format || !len) {
        res.ok = 0;
        return res;
    }

    slice_const s;
    size_t field_bytes = 0;
    cstr_fmt_float fmt_float;
    while (res.ok && bytes_written < len && *format != '\0') {
        switch (*format) {
        case 'c':
            dest[bytes_written] = (char)va_arg(va_args, int);
            field_bytes = 1;
            break;
        case 's':
            s = va_arg(va_args, slice_const);
            field_bytes = min(s.len, len - bytes_written);
            bytes_copy(dest + bytes_written, s.buffer, field_bytes);
            res.ok = s.len <= len - bytes_written;
            break;
        case 'S':
            s = slice_const_from_cstr_unsafe(va_arg(va_args, char *));
            field_bytes = min(s.len, len - bytes_written);
            bytes_copy(dest + bytes_written, s.buffer, field_bytes);
            res.ok = s.len <= len - bytes_written;
            break;
        case 'f':
            fmt_float.v = va_arg(va_args, double);
            fmt_float.precision = 6;
            field_bytes = cstr_from_double(
                dest + bytes_written,
                len - bytes_written,
                fmt_float.v,
                fmt_float.precision
            );
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'F':
            fmt_float = va_arg(va_args, cstr_fmt_float);
            field_bytes = cstr_from_double(
                dest + bytes_written,
                len - bytes_written,
                fmt_float.v,
                fmt_float.precision
            );
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'i':
            field_bytes = cstr_from_int(
                dest + bytes_written, len - bytes_written, va_arg(va_args, int)
            );
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'u':
            field_bytes = cstr_from_uint(
                dest + bytes_written, len - bytes_written, va_arg(va_args, uint)
            );
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'I':
            field_bytes = cstr_from_llong(
                dest + bytes_written,
                len - bytes_written,
                va_arg(va_args, llong)
            );
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'U':
            field_bytes = cstr_from_ullong(
                dest + bytes_written,
                len - bytes_written,
                va_arg(va_args, ullong)
            );
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        default:
            dest[bytes_written] = *format;
            field_bytes = 1;
            break;
        }
        bytes_written += field_bytes;
        format += 1;
    }

    // null termination
    if (bytes_written + 1 < len) {
        // null termination is not included in length
        dest[bytes_written] = '\0';
    } else {
        // always null terminate
        dest[len - 1] = '\0';
        bytes_written -= 1;
        res.ok = 0; // truncated
    }

    res.len = bytes_written;
    return res;
}

size_t cstr_fmt_len_va(const char *restrict format, va_list va_args) {
    assert(format && "format must not be null");

    if (!format) {
        return 0;
    }

    slice_const s;
    size_t len = 0;
    cstr_fmt_float fmt_float;
    while (*format != '\0') {
        switch (*format) {
        case 'c':
            len += 1;
            break;
        case 's':
            s = va_arg(va_args, slice_const);
            len += s.len;
            break;
        case 'S':
            s = slice_const_from_cstr_unsafe(va_arg(va_args, char *));
            len += s.len;
            break;
        case 'f':
            fmt_float.v = va_arg(va_args, double);
            fmt_float.precision = 6;
            len += cstr_len_double(fmt_float.v, fmt_float.precision);
            break;
        case 'F':
            fmt_float = va_arg(va_args, cstr_fmt_float);
            len += cstr_len_double(fmt_float.v, fmt_float.precision);
            break;
        case 'i':
            len += cstr_len_int(va_arg(va_args, int));
            break;
        case 'u':
            len += cstr_len_uint(va_arg(va_args, uint));
            break;
        case 'I':
            len += cstr_len_llong(va_arg(va_args, llong));
            break;
        case 'U':
            len += cstr_len_ullong(va_arg(va_args, ullong));
            break;
        default:
            len += 1;
            break;
        }
        format += 1;
    }

    return len;
}

////////////////////////
// Byte buffer
////////////////////////

void bytebuf_init(bytebuf *bbuf, size_t capacity, allocator *allocator) {
    assert(bbuf && "bytebuf must not be null");
    assert(allocator && "allocator must not be null");
    assert(capacity > 0 && "capacity must be larger than 0");
    bytes_set(bbuf, 0, sizeof(*bbuf));
    bbuf->allocator = allocator;
    bbuf->buffer = alloc_malloc(allocator, capacity, alignof(uchar));
    if (bbuf->buffer) {
        bbuf->cap = capacity;
    }
}

void bytebuf_init_fixed(
    bytebuf *bbuf, uchar *buffer, size_t len, size_t capacity
) {
    assert(bbuf && "bytebuf must not be null");
    bytes_set(bbuf, 0, sizeof(*bbuf));
    bbuf->buffer = buffer;
    bbuf->len = len;
    bbuf->cap = capacity;
}

void bytebuf_free(bytebuf *bbuf) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(bbuf->allocator && "allocator must not be null");
    if (!bbuf && !bbuf->buffer) {
        return;
    }
    if (bbuf->allocator) {
        alloc_free(bbuf->allocator, bbuf->buffer);
    }
    bbuf->cap = 0;
    bbuf->len = 0;
    bbuf->buffer = 0;
}

bool bytebuf_grow(bytebuf *bbuf, size_t capacity_increase) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->allocator && "allocator must not be null");
    assert(capacity_increase > 0 && "capacity increase must be larger than 0");
    if (capacity_increase == 0) {
        return 1;
    }
    if (!bbuf->allocator) {
        return 0;
    }

    uchar *newbuf = alloc_malloc(
        bbuf->allocator, bbuf->cap + capacity_increase, alignof(uchar)
    );
    if (!newbuf) {
        return 0;
    }

    if (bbuf->buffer) {
        bytes_copy(newbuf, bbuf->buffer, bbuf->len);
    }
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

bool bytebuf_write(bytebuf *bbuf, const uchar *src, size_t len) {
    assert(bbuf && "bytebuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");
    assert(src && "source must not be null");
    assert(len > 0 && "length must be more than 0");

    if (!src || len == 0) {
        return 1; // nothing to write
    }
    if (len > bytebuf_bytes_available(bbuf)) {
        if (bytebuf_is_growable(bbuf)) {
            size_t capacity_increase = bbuf->cap + len;
            bool ok = bytebuf_grow(bbuf, capacity_increase);
            if (!ok) {
                return 0; // grow failed
            }
        } else {
            return 0; // no capacity left
        }
    }
    bytes_copy(bbuf->buffer + bbuf->len, src, len);
    bbuf->len += len;
    return 1;
}

size_t bytebuf_write_int(bytebuf *bbuf, int src) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");

    char tmp[16];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_int_unsafe(end, src);
    if (bytebuf_write(bbuf, (uchar *)end - bytes_to_copy, bytes_to_copy)) {
        return bytes_to_copy;
    }
    return 0;
}

size_t bytebuf_write_uint(bytebuf *bbuf, uint src) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");

    char tmp[16];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_uint_unsafe(end, src);
    if (bytebuf_write(bbuf, (uchar *)end - bytes_to_copy, bytes_to_copy)) {
        return bytes_to_copy;
    }
    return 0;
}

size_t bytebuf_write_llong(bytebuf *bbuf, llong src) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");

    char tmp[32];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_llong_unsafe(end, src);
    if (bytebuf_write(bbuf, (uchar *)end - bytes_to_copy, bytes_to_copy)) {
        return bytes_to_copy;
    }
    return 0;
}

size_t bytebuf_write_ullong(bytebuf *bbuf, ullong src) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");

    char tmp[32];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_ullong_unsafe(end, src);
    if (bytebuf_write(bbuf, (uchar *)end - bytes_to_copy, bytes_to_copy)) {
        return bytes_to_copy;
    }
    return 0;
}

size_t bytebuf_write_float(bytebuf *bbuf, float src, uint decimals) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");

    char integer_cstr[32];
    char fractional_cstr[32];
    struct cstr_from_real_parts parts = {0};
    parts.integer_cursor = integer_cstr + sizeof(integer_cstr);
    parts.fractional_cursor = fractional_cstr + sizeof(fractional_cstr);
    cstr_from_float_parts(&parts, src, decimals);
    size_t bytes_to_write = cstr_from_real_parts_len(&parts);

    if (bytes_to_write > bytebuf_bytes_available(bbuf)) {
        if (bytebuf_is_growable(bbuf)) {
            size_t capacity_increase = bbuf->cap + bytes_to_write * 2;
            bool ok = bytebuf_grow(bbuf, capacity_increase);
            if (!ok) {
                return 0; // grow failed
            }
        } else {
            return 0; // no capacity left
        }
    }

    cstr_from_real_parts_to_buf(&parts, (char *)bbuf->buffer + bbuf->len);
    bbuf->len += bytes_to_write;
    return bytes_to_write;
}

size_t bytebuf_write_double(bytebuf *bbuf, double src, uint decimals) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bytebuf's buffer must not be null");

    char integer_cstr[32];
    char fractional_cstr[32];
    struct cstr_from_real_parts parts = {0};
    parts.integer_cursor = integer_cstr + sizeof(integer_cstr);
    parts.fractional_cursor = fractional_cstr + sizeof(fractional_cstr);
    cstr_from_double_parts(&parts, src, decimals);
    size_t bytes_to_write = cstr_from_real_parts_len(&parts);

    if (bytes_to_write > bytebuf_bytes_available(bbuf)) {
        if (bytebuf_is_growable(bbuf)) {
            size_t capacity_increase = bbuf->cap + bytes_to_write * 2;
            bool ok = bytebuf_grow(bbuf, capacity_increase);
            if (!ok) {
                return 0; // grow failed
            }
        } else {
            return 0; // no capacity left
        }
    }

    cstr_from_real_parts_to_buf(&parts, (char *)bbuf->buffer + bbuf->len);
    bbuf->len += bytes_to_write;
    return bytes_to_write;
}

cstr_fmt_result
bytebuf_fmt_va(bytebuf *bbuf, const char *restrict format, va_list va_args) {
    assert(bbuf && "bbuf must not be null");
    assert(bbuf->buffer && "bbuf's buffer must not be null");
    assert(format && "format must not be null");

    cstr_fmt_result res = {
        .ok = 1,
        .len = 0,
    };

    if (!format) {
        res.ok = 0;
        return res;
    }

    slice_const s;
    char c;
    size_t field_bytes = 0;
    cstr_fmt_float fmt_float;
    while (res.ok && *format != '\0') {
        switch (*format) {
        case 'c':
            c = (char)va_arg(va_args, int);
            res.ok = bytebuf_write(bbuf, (const uchar *)&c, 1);
            field_bytes = 1;
            break;
        case 's':
            s = va_arg(va_args, slice_const);
            res.ok = bytebuf_write(bbuf, s.buffer, s.len);
            field_bytes = s.len;
            break;
        case 'S':
            s = slice_const_from_cstr_unsafe(va_arg(va_args, char *));
            res.ok = bytebuf_write(bbuf, s.buffer, s.len);
            field_bytes = s.len;
            break;
        case 'f':
            fmt_float.v = va_arg(va_args, double);
            fmt_float.precision = 6;
            field_bytes =
                bytebuf_write_double(bbuf, fmt_float.v, fmt_float.precision);
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'F':
            fmt_float = va_arg(va_args, cstr_fmt_float);
            field_bytes =
                bytebuf_write_double(bbuf, fmt_float.v, fmt_float.precision);
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'i':
            field_bytes = bytebuf_write_int(bbuf, va_arg(va_args, int));
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'u':
            field_bytes = bytebuf_write_uint(bbuf, va_arg(va_args, uint));
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'I':
            field_bytes = bytebuf_write_llong(bbuf, va_arg(va_args, llong));
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        case 'U':
            field_bytes = bytebuf_write_ullong(bbuf, va_arg(va_args, ullong));
            if (field_bytes == 0) {
                res.ok = 0;
            }
            break;
        default:
            res.ok = bytebuf_write(bbuf, (const uchar *)format, 1);
            field_bytes = 1;
            break;
        }
        if (res.ok) {
            res.len += field_bytes;
        }
        format += 1;
    }

    // null termination
    res.ok = bytebuf_write(bbuf, (const uchar *)"\0", 1);
    if (res.ok) {
        // null termination is not included in length
        bbuf->len -= 1;
    } else {
        // always null terminate
        bbuf->buffer[bbuf->len - 1] = '\0';
        bbuf->len -= 1;
        res.len -= 1;
    }

    return res;
}

////////////////////////
// Buffered byte stream
////////////////////////

bytesink_result bufstream_flush(bufstream *bstream) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bstream's buffer must not be null");

    bytesink_result res = {0};

    if (bstream->len == 0) {
        return res;
    }

    res =
        bstream->sink.fn(bstream->sink.context, bstream->buffer, bstream->len);

    if (res.len >= bstream->len) {
        // full write successful
        bstream->len = 0;
        return res;
    }

    // partial write --> move remaining bytes to beginning of buffer
    bytes_move(bstream->buffer, bstream->buffer + res.len, 0);
    bstream->len -= res.len;

    return res;
}

bufstream_write_result
bufstream_write(bufstream *bstream, const uchar *src, size_t len) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bstream's buffer must not be null");
    assert(src && "source must not be null");
    assert(bstream->sink.fn && "sink fn must not be null");

    bufstream_write_result res = {0};

    if (!src || len == 0) {
        // nothing to write
        return res;
    }

    while (res.len < len) {
        size_t bytes_available = len - res.len;

        // buffer empty and available bytes exceed buffer capacity
        // --> pipe to sink directly
        if (bstream->len == 0 && bytes_available > bstream->cap) {
            bytesink_result bs_res = bstream->sink.fn(
                bstream->sink.context, src + res.len, bytes_available
            );
            res.err_code = bs_res.err_code;
            res.len += bs_res.len;
            return res;
        }

        // append whatever we can to the buffer
        size_t bytes_to_copy =
            min(bytes_available, bstream->cap - bstream->len);
        bytes_copy(
            bstream->buffer + bstream->len, src + res.len, bytes_to_copy
        );
        bstream->len += bytes_to_copy;
        res.len += bytes_to_copy;

        // no space left --> flush buffer to sink
        assert(bstream->cap >= bstream->len);
        if (bstream->cap == bstream->len) {
            bytesink_result bs_res = bufstream_flush(bstream);
            res.err_code = bs_res.err_code;
            if (res.err_code) {
                return res;
            }
        }
    }

    return res;
}

bufstream_write_result bufstream_write_int(bufstream *bstream, int src) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bufstream's buffer must not be null");

    char tmp[16];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_int_unsafe(end, src);
    return bufstream_write(
        bstream, (uchar *)end - bytes_to_copy, bytes_to_copy
    );
}

bufstream_write_result bufstream_write_uint(bufstream *bstream, uint src) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bufstream's buffer must not be null");

    char tmp[16];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_uint_unsafe(end, src);
    return bufstream_write(
        bstream, (uchar *)end - bytes_to_copy, bytes_to_copy
    );
}

bufstream_write_result bufstream_write_llong(bufstream *bstream, llong src) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bufstream's buffer must not be null");

    char tmp[32];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_llong_unsafe(end, src);
    return bufstream_write(
        bstream, (uchar *)end - bytes_to_copy, bytes_to_copy
    );
}

bufstream_write_result bufstream_write_ullong(bufstream *bstream, ullong src) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bufstream's buffer must not be null");

    char tmp[32];
    char *end = tmp + sizeof(tmp);
    size_t bytes_to_copy = cstr_from_ullong_unsafe(end, src);
    return bufstream_write(
        bstream, (uchar *)end - bytes_to_copy, bytes_to_copy
    );
}

static bufstream_write_result cstr_from_real_parts_to_bufstream(
    struct cstr_from_real_parts *parts, bufstream *bstream
) {
    // infinity
    if (parts->is_inf) {
        const char *text = "inf";
        size_t bytes_to_write = sizeof("inf") - 1;
        if (parts->is_neg) {
            text = "-inf";
            bytes_to_write = sizeof("-inf") - 1;
        }
        return bufstream_write(bstream, (const uchar *)text, bytes_to_write);
    }

    // negative sign
    bufstream_write_result res;
    size_t integer_len = parts->integer_len;
    if (parts->is_neg) {
        // inject the negative sign to the integer buffer
        integer_len += 1;
        *(parts->integer_cursor - integer_len) = '-';
    }

    // integer part
    res = bufstream_write(
        bstream, (uchar *)parts->integer_cursor - integer_len, integer_len
    );
    if (res.err_code) {
        return res;
    }

    // decimal + decimal zeros
    uchar decimals[32];
    decimals[0] = '.';
    for (uint i = 0; i < parts->decimal_zeros; i += 1) {
        decimals[i + 1] = '0';
    }
    res = bufstream_write(bstream, decimals, parts->decimal_zeros + 1);

    // fractional part
    res = bufstream_write(
        bstream,
        (uchar *)parts->fractional_cursor - parts->fractional_len,
        parts->fractional_len
    );

    return res;
}

bufstream_write_result
bufstream_write_float(bufstream *bstream, float src, uint decimals) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bufstream's buffer must not be null");

    char integer_cstr[32];
    char fractional_cstr[32];
    struct cstr_from_real_parts parts = {0};
    parts.integer_cursor = integer_cstr + sizeof(integer_cstr);
    parts.fractional_cursor = fractional_cstr + sizeof(fractional_cstr);
    cstr_from_float_parts(&parts, src, decimals);
    return cstr_from_real_parts_to_bufstream(&parts, bstream);
}

bufstream_write_result
bufstream_write_double(bufstream *bstream, double src, uint decimals) {
    assert(bstream && "bstream must not be null");
    assert(bstream->buffer && "bufstream's buffer must not be null");

    char integer_cstr[32];
    char fractional_cstr[32];
    struct cstr_from_real_parts parts = {0};
    parts.integer_cursor = integer_cstr + sizeof(integer_cstr);
    parts.fractional_cursor = fractional_cstr + sizeof(fractional_cstr);
    cstr_from_double_parts(&parts, src, decimals);
    return cstr_from_real_parts_to_bufstream(&parts, bstream);
}

bufstream_write_result bufstream_fmt_va(
    bufstream *bstream, const char *restrict format, va_list va_args
) {
    assert(bstream && "bufstream must not be null");
    assert(format && "format must not be null");

    bufstream_write_result res = {
        .err_code = 0,
        .len = 0,
    };

    if (!format) {
        return res;
    }

    slice_const s;
    char c;
    cstr_fmt_float fmt_float;
    bufstream_write_result temp_res;
    while (res.err_code == 0 && *format != '\0') {
        switch (*format) {
        case 'c':
            c = (char)va_arg(va_args, int);
            temp_res = bufstream_write(bstream, (const uchar *)&c, 1);
            break;
        case 's':
            s = va_arg(va_args, slice_const);
            temp_res = bufstream_write(bstream, s.buffer, s.len);
            break;
        case 'S':
            s = slice_const_from_cstr_unsafe(va_arg(va_args, char *));
            temp_res = bufstream_write(bstream, s.buffer, s.len);
            break;
        case 'f':
            fmt_float.v = va_arg(va_args, double);
            fmt_float.precision = 6;
            temp_res = bufstream_write_double(
                bstream, fmt_float.v, fmt_float.precision
            );
            break;
        case 'F':
            fmt_float = va_arg(va_args, cstr_fmt_float);
            temp_res = bufstream_write_double(
                bstream, fmt_float.v, fmt_float.precision
            );
            break;
        case 'i':
            temp_res = bufstream_write_int(bstream, va_arg(va_args, int));
            break;
        case 'u':
            temp_res = bufstream_write_uint(bstream, va_arg(va_args, uint));
            break;
        case 'I':
            temp_res = bufstream_write_llong(bstream, va_arg(va_args, llong));
            break;
        case 'U':
            temp_res = bufstream_write_ullong(bstream, va_arg(va_args, ullong));
            break;
        default:
            temp_res = bufstream_write(bstream, (const uchar *)format, 1);
            break;
        }
        res.err_code = temp_res.err_code;
        if (temp_res.err_code == 0) {
            res.len += temp_res.len;
        }
        format += 1;
    }

    return res;
}
