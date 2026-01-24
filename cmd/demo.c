#include "io.h"
#include "std.h"
#include <string.h>
#include <unistd.h>

int array_demo(void) {
    int exit_code = 0;

    slice buffer = alloc_new(&std_allocator, uchar, 1024 * 1024);
    arena arena = arena_new(buffer.buffer, 1024 * 1024);
    allocator allocator = arena_allocator_new(&arena);

    float *arr = dynarr_new(10, float, &allocator);
    float last = 0.0;
    ullong i;

    for (i = 0; i < 200; i += 10) {
        float v = ((float)i) / 10;
        float *new_arr = dynarr_push_grow(arr, &v, 1, float);
        if (new_arr) {
            arr = new_arr;
        } else {
            io_stderr_write_sstr("allocation failed!\n");
            exit_code = 1;
            goto end;
        }
    }

    dynarr_remove(arr, 9);
    dynarr_remove(arr, 12);
    dynarr_pop(arr, last);

    for (i = 0; i < dynarr_len(arr); i += 1) {
        io_stdout_fmt("s U: f\n", slice_sstr("data"), i, arr[i]);
    }

    io_stdout_fmt(
        "u: f, u: f, s: f\n", 2, arr[2], 15, arr[15], slice_sstr("last"), last
    );
end:
    dynarr_free(arr);
    arena_clear(&arena);
    alloc_free(&std_allocator, buffer);

    return exit_code;
}

void print_file_error(const char *filename, int err_code) {
    if (err_code == file_err_invalid_stat) {
        io_stderr_fmt(
            "s S\n", slice_sstr("Failed to get details for file"), filename
        );
    } else if (err_code == file_err_failed_alloc) {
        io_stderr_fmt(
            "s S\n", slice_sstr("Failed to allocate memory for file"), filename
        );
    } else {
        const char *err_msg = strerror(err_code);
        io_stderr_fmt(
            "s S: S\n", slice_sstr("File error for file"), filename, err_msg
        );
    }
}

int file_demo(int argc, char **argv) {
    assert(argc > 1 && "expected at least one cli param");

    const char *filename_read = argv[1];

    file_read_result read_res = file_read_sync(filename_read, &std_allocator);
    if (read_res.err_code) {
        print_file_error(filename_read, read_res.err_code);
        return 1;
    }

    if (argc > 2) {
        const char *filename_write = argv[2];
        io_result write_res =
            file_write_sync(filename_write, read_res.data.buffer, read_res.data.len);
        if (write_res.err_code) {
            print_file_error(filename_write, write_res.err_code);
            return 1;
        }
    } else {
        io_stdout_fmt(
            "s: U\ns:\ns\n",
            slice_sstr("File size"),
            read_res.data.len,
            slice_sstr("File contents"),
            read_res.data
        );
    }

    if (slice_is_set(read_res.data)) {
        file_read_result_free(read_res, &std_allocator);
    }
    return 0;
}

int main(int argc, char **argv) {
    int ret_code = 0;
    if (argc > 1) {
        ret_code = file_demo(argc, argv);
    } else {
        ret_code = array_demo();
    }

    io_stdout_flush();
    io_stderr_flush();

    return ret_code;
}
