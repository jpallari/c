#include "std.h"
#include "testr.h"
#include <limits.h>

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
    cstr_split_iter split;
    cstr_split_init(&split, slice_str(str), slice_sstr(" "), 0);

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

    cstr_split_iter split;
    cstr_split_init(&split, slice_str(str), slice_sstr(" "), 0);

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
    cstr_split_iter split;
    cstr_split_init(
        &split, slice_str(str), slice_sstr(" "), cstr_split_flag_null_terminate
    );

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

    cstr_split_iter split;
    cstr_split_init(&split, slice_str(str), slice_sstr(" "), 0);

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
        t,
        bitset_is_set(split.flags, cstr_split_flag_null_terminate),
        "null termination flag is toggled back"
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
        t,
        bitset_is_set(split.flags, cstr_split_flag_null_terminate),
        "null termination flag is toggled back"
    );
}

void test_cstr_to_int(test *t) {
    int v = 0;
    assert_eq_uint(t, cstr_to_int("127", 3, &v), 3, "convert exact length");
    assert_eq_sint(t, v, 127, "value after conversion");
    assert_eq_uint(t, cstr_to_int("1279", 3, &v), 3, "convert short length");
    assert_eq_sint(t, v, 127, "value after conversion");
    assert_eq_uint(
        t, cstr_to_int("12\0 123", 4, &v), 2, "convert null terminated"
    );
    assert_eq_sint(t, v, 12, "value after conversion");
    assert_eq_uint(t, cstr_to_int("-127", 4, &v), 4, "convert negative");
    assert_eq_sint(t, v, -127, "value after conversion");
    assert_eq_uint(t, cstr_to_int("0", 1, &v), 1, "convert 0");
    assert_eq_sint(t, v, 0, "value after conversion");
    assert_eq_uint(t, cstr_to_int("2147483647", 11, &v), 10, "convert max int");
    assert_eq_sint(t, v, 2147483647, "value after conversion");
    assert_eq_uint(
        t, cstr_to_int("-2147483647", 12, &v), 11, "convert min int"
    );
    assert_eq_sint(t, v, -2147483647, "value after conversion");
    assert_eq_uint(t, cstr_to_int("102a", 4, &v), 3, "convert invalid chars");
    assert_eq_sint(t, v, 102, "prefix is converted");

    v = 0;
    assert_false(t, cstr_to_int("2147483648", 11, &v), "convert too long int");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_int("a110", 4, &v), "convert invalid chars");
    assert_false(t, v, "value remains unchanged");
}

void test_cstr_to_uint(test *t) {
    uint v = 0;
    assert_eq_uint(t, cstr_to_uint("255", 3, &v), 3, "convert exact length");
    assert_eq_uint(t, v, 255, "value after conversion");
    assert_eq_uint(t, cstr_to_uint("2559", 3, &v), 3, "convert short length");
    assert_eq_uint(t, v, 255, "value after conversion");
    assert_eq_uint(
        t, cstr_to_uint("24\0 123", 4, &v), 2, "convert null terminated"
    );
    assert_eq_uint(t, v, 24, "value after conversion");
    assert_eq_uint(t, cstr_to_uint("0", 1, &v), 1, "convert 0");
    assert_eq_uint(t, v, 0, "value after conversion");
    assert_eq_uint(
        t, cstr_to_uint("4294967295", 11, &v), 10, "convert max int"
    );
    assert_eq_uint(t, v, 4294967295, "value after conversion");
    assert_eq_uint(t, cstr_to_uint("102a", 4, &v), 3, "convert invalid chars");
    assert_eq_uint(t, v, 102, "prefix is converted");

    v = 0;
    assert_false(t, cstr_to_uint("4294967296", 11, &v), "convert too long int");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_uint("-1", 4, &v), "convert negative number");
    assert_false(t, v, "value remains unchanged");
    assert_false(t, cstr_to_uint("a110", 4, &v), "convert invalid chars");
    assert_false(t, v, "value remains unchanged");
}

void test_cstr_from_int(test *t) {
    char buffer[100];
    bytes_set(buffer, 0xce, sizeof(buffer));

    assert_eq_uint(
        t, cstr_from_int_nt(buffer, sizeof(buffer), 120), 4, "write all chars"
    );
    assert_eq_cstr(t, buffer, "120", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_int_nt(buffer, sizeof(buffer), 89), 3, "write all chars"
    );
    assert_eq_cstr(t, buffer, "89", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_int_nt(buffer, sizeof(buffer), 1), 2, "write all chars"
    );
    assert_eq_cstr(t, buffer, "1", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_int_nt(buffer, sizeof(buffer), 0), 2, "write all chars"
    );
    assert_eq_cstr(t, buffer, "0", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_int_nt(buffer, sizeof(buffer), -127), 5, "write all chars"
    );
    assert_eq_cstr(t, buffer, "-127", "expected number as string");

    bytes_copy(buffer, "xx", 3);
    assert_eq_uint(t, cstr_from_int_nt(buffer, 2, 123), 0, "write no chars");
    assert_eq_bytes(t, buffer, "xx", 3, "expected no changes");
}

void test_cstr_from_uint(test *t) {
    char buffer[100];
    bytes_set(buffer, 0xce, sizeof(buffer));

    assert_eq_uint(
        t, cstr_from_uint_nt(buffer, sizeof(buffer), 254), 4, "write all chars"
    );
    assert_eq_cstr(t, buffer, "254", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_uint_nt(buffer, sizeof(buffer), 89), 3, "write all chars"
    );
    assert_eq_cstr(t, buffer, "89", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_uint_nt(buffer, sizeof(buffer), 1), 2, "write all chars"
    );
    assert_eq_cstr(t, buffer, "1", "expected number as string");

    bytes_set(buffer, 0xce, sizeof(buffer));
    assert_eq_uint(
        t, cstr_from_uint_nt(buffer, sizeof(buffer), 0), 2, "write all chars"
    );
    assert_eq_cstr(t, buffer, "0", "expected number as string");

    bytes_copy(buffer, "xx", 3);
    assert_eq_uint(t, cstr_from_uint_nt(buffer, 2, 123), 0, "write no chars");
    assert_eq_bytes(t, buffer, "xx", 3, "expected no changes");
}

void test_cstr_fmt(test *t) {
    char buf[1024] = {0};
    size_t len;
    cstr_fmt_result res;
    const char *format;

    format = "s: s; s: i";
    len = cstr_fmt_len(
        format, slice_sstr("string"), slice_sstr("hello"), slice_sstr("int"), -4
    );
    assert_eq_uint(t, len, 22, "1: fmt len");
    res = cstr_fmt(
        buf,
        sizeof(buf),
        format,
        slice_sstr("string"),
        slice_sstr("hello"),
        slice_sstr("int"),
        -4
    );
    assert_eq_cstr(t, buf, "string: hello; int: -4", "1: fmt contents");
    assert_eq_uint(t, res.len, 22, "1: fmt content len");
    assert_true(t, res.ok, "1: ok");

    format = "s: U";
    len = cstr_fmt_len(format, slice_sstr("ullong"), 123UL);
    assert_eq_uint(t, len, 11, "2: fmt len");
    res = cstr_fmt(buf, sizeof(buf), format, slice_sstr("ullong"), 123UL);
    assert_eq_cstr(t, buf, "ullong: 123", "2: fmt contents");
    assert_eq_uint(t, res.len, 11, "2: fmt content len");
    assert_true(t, res.ok, "2: ok");

    format = "u; I";
    len = cstr_fmt_len(format, 98, -12345L);
    assert_eq_uint(t, len, 10, "3: fmt len");
    res = cstr_fmt(buf, sizeof(buf), format, 98, -12345L);
    assert_eq_cstr(t, buf, "98; -12345", "3: fmt contents");
    assert_eq_uint(t, res.len, 10, "3: fmt content len");
    assert_true(t, res.ok, "3: ok");

    format = "F; F";
    len = cstr_fmt_len(
        format,
        (cstr_fmt_float) {.v = (double)2.456f, .precision = 2},
        (cstr_fmt_float) {.v = 123.456789, .precision = 4}
    );
    assert_eq_uint(t, len, 14, "4: fmt len");
    res = cstr_fmt(
        buf,
        sizeof(buf),
        format,
        (cstr_fmt_float) {.v = (double)2.456f, .precision = 2},
        (cstr_fmt_float) {.v = 123.456789, .precision = 4}
    );
    assert_eq_cstr(t, buf, "2.46; 123.4568", "4: fmt contents");
    assert_eq_uint(t, res.len, 14, "4: fmt content len");
    assert_true(t, res.ok, "4: ok");

    format = "s: s; s: c!";
    len = cstr_fmt_len(
        format,
        slice_sstr("string"),
        slice_const_new("hello world", 5),
        slice_sstr("char"),
        'x'
    );
    assert_eq_uint(t, len, 23, "5: fmt len");
    res = cstr_fmt(
        buf,
        sizeof(buf),
        format,
        slice_sstr("string"),
        slice_const_new("hello world", 5),
        slice_sstr("char"),
        'x'
    );
    assert_eq_cstr(t, buf, "string: hello; char: x!", "5: fmt contents");
    assert_eq_uint(t, res.len, 23, "5: fmt content len");
    assert_true(t, res.ok, "5: ok");

    format = "s: h;";
    len = cstr_fmt_len(format, slice_sstr("hex"), slice_sstr("abcd"));
    assert_eq_uint(t, len, 14, "6: fmt len");
    res = cstr_fmt(
        buf, sizeof(buf), format, slice_sstr("hex"), slice_sstr("abcd")
    );
    assert_eq_cstr(t, buf, "hex: 61626364;", "6: fmt contents");
    assert_eq_uint(t, res.len, 14, "6: fmt content len");
    assert_true(t, res.ok, "6: ok");
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
    {"C string to int", test_cstr_to_int},
    {"C string to uint", test_cstr_to_uint},
    {"C string from int", test_cstr_from_int},
    {"C string from uint", test_cstr_from_uint},
    {"C string fmt", test_cstr_fmt}
};

setup_tests(NULL, tests)
