#include "std.h"
#include "testr.h"

void test_cstr_eq(test *t) {
    assert_true(t, cstr_eq("hello", "hello", 5), "same C strings are equal");
    assert_false(
        t, cstr_eq("foo", "bar", 3), "different C strings are not equal"
    );
    assert_true(
        t,
        cstr_eq("hello", "hello world", 5),
        "C strings are equal when their prefixes match until the given length"
    );
}

void test_cstr_eq_unsafe(test *t) {
    assert_true(
        t, cstr_eq_unsafe("hello", "hello"), "same C strings are equal"
    );
    assert_false(
        t, cstr_eq_unsafe("foo", "bar"), "different C strings are not equal"
    );
}

void test_cstr_len(test *t) {
    assert_eq_uint(
        t,
        cstr_len("hello", 6),
        5L,
        "length should be counted until null terminator"
    );
    assert_eq_uint(
        t, cstr_len("hello", 3), 3L, "length should be up to the capacity"
    );
}

void test_cstr_len_unsafe(test *t) {
    assert_eq_uint(
        t,
        cstr_len_unsafe("hello"),
        5L,
        "length should be counted until null terminator"
    );
}

void test_cstr_split(test *t) {
    char str[] = "this is a string";
    cstr_split_iter split = {
        .str = str,
        .str_len = sizeof(str),
        .split_chars = " ",
        .split_chars_len = 1,
        .index = 0,
        .null_terminate = 0,
    };

    // first word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_uint(t, slice.len, 4L, "1 - length");
        assert_eq_bytes(t, slice.buffer, "this", slice.len, "1 - contents");
    }

    // second word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_uint(t, slice.len, 2L, "2 - length");
        assert_eq_bytes(t, slice.buffer, "is", slice.len, "2 - contents");
    }

    // third word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_uint(t, slice.len, 1L, "3 - length");
        assert_eq_bytes(t, slice.buffer, "a", slice.len, "3 - contents");
    }

    // fourth word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_uint(t, slice.len, 6L, "4 - length");
        assert_eq_bytes(t, slice.buffer, "string", slice.len, "4 - contents");
    }

    // fifth iteration
    {
        slice slice = cstr_split_next(&split);
        assert_false(t, slice.len, "5 - length");
        assert_false(t, slice.buffer, "4 - contents");
    }
}

void test_cstr_split_collect(test *t) {
    char str[] = "collecting all words to an array";

    cstr_split_iter split = {
        .str = str,
        .str_len = sizeof(str),
        .split_chars = " ",
        .split_chars_len = 1,
        .index = 0,
        .null_terminate = 0,
    };

    slice arr[10] = {0};

    // collect all sub-strings
    {
        size_t len = cstr_split_collect(arr, countof(arr), &split);
        assert_eq_uint(t, len, 6L, "1 - length");
        assert_eq_bytes(
            t, arr[0].buffer, "collecting", arr[0].len, "1 index 0 - contents"
        );
        assert_eq_bytes(
            t, arr[1].buffer, "all", arr[1].len, "1 index 1 - contents"
        );
        assert_eq_bytes(
            t, arr[2].buffer, "words", arr[2].len, "1 index 2 - contents"
        );
        assert_eq_bytes(
            t, arr[3].buffer, "to", arr[3].len, "1 index 3 - contents"
        );
        assert_eq_bytes(
            t, arr[4].buffer, "an", arr[4].len, "1 index 4 - contents"
        );
        assert_eq_bytes(
            t, arr[5].buffer, "array", arr[5].len, "1 index 5 - contents"
        );
    }

    bytes_set(arr, 0, sizeof(arr));
    split.index = 0;

    // collect some sub-strings
    {
        size_t len = cstr_split_collect(arr, 3, &split);
        assert_eq_uint(t, len, 3L, "2 - length");
        assert_eq_bytes(
            t, arr[0].buffer, "collecting", arr[0].len, "2 index 0 - contents"
        );
        assert_eq_bytes(
            t, arr[1].buffer, "all", arr[1].len, "2 index 1 - contents"
        );
        assert_eq_bytes(
            t, arr[2].buffer, "words", arr[2].len, "2 index 2 - contents"
        );
    }
}

void test_cstr_split_null_terminate(test *t) {
    char str[] = "null terminate these words";
    cstr_split_iter split = {
        .str = str,
        .str_len = sizeof(str),
        .split_chars = " ",
        .split_chars_len = 1,
        .index = 0,
        .null_terminate = 1,
    };

    // first word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_cstr(t, (char *)slice.buffer, "null", "1 - contents");
    }

    // second word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_cstr(t, (char *)slice.buffer, "terminate", "2 - contents");
    }

    // third word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_cstr(t, (char *)slice.buffer, "these", "3 - contents");
    }

    // fourth word
    {
        slice slice = cstr_split_next(&split);
        assert_eq_cstr(t, (char *)slice.buffer, "words", "4 - contents");
    }

    // fifth iteration
    {
        slice slice = cstr_split_next(&split);
        assert_false(t, slice.len, "5 - length");
        assert_false(t, slice.buffer, "4 - contents");
    }
}

void test_cstr_split_collect_strings(test *t) {
    char str[] = "collecting all words to an array";

    cstr_split_iter split = {
        .str = str,
        .str_len = sizeof(str),
        .split_chars = " ",
        .split_chars_len = 1,
        .index = 0,
        .null_terminate = 0,
    };

    char *arr[10] = {0};

    // collect all sub-strings
    {
        size_t len = cstr_split_collect_strings(arr, countof(arr), &split);
        assert_eq_uint(t, len, 6L, "1 - length");
        assert_eq_cstr(t, arr[0], "collecting", "1 index 0 - contents");
        assert_eq_cstr(t, arr[1], "all", "1 index 1 - contents");
        assert_eq_cstr(t, arr[2], "words", "1 index 2 - contents");
        assert_eq_cstr(t, arr[3], "to", "1 index 3 - contents");
        assert_eq_cstr(t, arr[4], "an", "1 index 4 - contents");
        assert_eq_cstr(t, arr[5], "array", "1 index 5 - contents");
    }

    assert_false(
        t, split.null_terminate, "null termination flag is toggled back"
    );
    bytes_set(arr, 0, sizeof(arr));
    split.index = 0;

    // collect some sub-strings
    {
        size_t len = cstr_split_collect_strings(arr, 3, &split);
        assert_eq_uint(t, len, 3L, "2 - length");
        assert_eq_cstr(t, arr[0], "collecting", "2 index 0 - contents");
        assert_eq_cstr(t, arr[1], "all", "2 index 1 - contents");
        assert_eq_cstr(t, arr[2], "words", "2 index 2 - contents");
    }

    assert_false(
        t, split.null_terminate, "null termination flag is toggled back"
    );
}

void test_cstr_to_s8(test *t) {
    s8 v = 0;
    assert_true(t, cstr_to_s8("127", 3, &v), "convert exact length");
    assert_eq_sint(t, v, 127, "value after conversion");
    assert_true(t, cstr_to_s8("1279", 3, &v), "convert short length");
    assert_eq_sint(t, v, 127, "value after conversion");
    assert_true(t, cstr_to_s8("12\0 123", 4, &v), "convert null terminated");
    assert_eq_sint(t, v, 12, "value after conversion");
    assert_true(t, cstr_to_s8("-127", 4, &v), "convert negative");
    assert_eq_sint(t, v, -127, "value after conversion");
    assert_true(t, cstr_to_s8("0", 1, &v), "convert 0");
    assert_eq_sint(t, v, 0, "value after conversion");

    v = 0;
    assert_false(t, cstr_to_s8("128", 3, &v), "convert too long int");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_s8("a110", 4, &v), "convert invalid chars");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_s8("102a", 4, &v), "convert invalid chars");
    assert_false(t, v, "value remains unchanged");
}

void test_cstr_to_u8(test *t) {
    u8 v = 0;
    assert_true(t, cstr_to_u8("255", 3, &v), "convert exact length");
    assert_eq_sint(t, v, 255, "value after conversion");
    assert_true(t, cstr_to_u8("2559", 3, &v), "convert short length");
    assert_eq_sint(t, v, 255, "value after conversion");
    assert_true(t, cstr_to_u8("24\0 123", 4, &v), "convert null terminated");
    assert_eq_sint(t, v, 24, "value after conversion");
    assert_true(t, cstr_to_u8("0", 1, &v), "convert 0");
    assert_eq_sint(t, v, 0, "value after conversion");

    v = 0;
    assert_false(t, cstr_to_u8("256", 3, &v), "convert too long int");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_u8("-1", 4, &v), "convert negative number");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_u8("a110", 4, &v), "convert invalid chars");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_u8("102a", 4, &v), "convert invalid chars");
    assert_false(t, v, "value remains unchanged");
}

static test_case tests[] = {
    {"C string equals", test_cstr_eq},
    {"C string equals (unsafe)", test_cstr_eq_unsafe},
    {"C string length", test_cstr_len},
    {"C string length (unsafe)", test_cstr_len_unsafe},
    {"C string split", test_cstr_split},
    {"C string split collect", test_cstr_split_collect},
    {"C string split null-terminate", test_cstr_split_null_terminate},
    {"C string split collect strings", test_cstr_split_collect_strings},
    {"C string to s8", test_cstr_to_s8},
    {"C string to u8", test_cstr_to_u8}
};

setup_tests(NULL, tests)
