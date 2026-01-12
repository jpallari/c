#include "io.h"
#include "std.h"
#include <string.h>
#include <unistd.h>

int array_demo(bufstream *stdout) {
    int exit_code = 0;

    uchar *buffer = alloc_new(&std_allocator, uchar, 1024 * 1024);
    arena arena = arena_new(buffer, 1024 * 1024);
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
            io_write_str_sync(STDERR_FILENO, "allocation failed!\n");
            exit_code = 1;
            goto end;
        }
    }

    dynarr_remove(arr, 9);
    dynarr_remove(arr, 12);
    dynarr_pop(arr, last);

    for (i = 0; i < dynarr_len(arr); i += 1) {
        bufstream_fmt(stdout, "s U: f\n", slice_sstr("data"), i, arr[i]);
    }

    bufstream_fmt(
        stdout,
        "u: f, u: f, s: f\n",
        2,
        arr[2],
        15,
        arr[15],
        slice_sstr("last"),
        last
    );
end:
    dynarr_free(arr);
    arena_clear(&arena);
    alloc_free(&std_allocator, buffer);

    return exit_code;
}

void print_file_error(bufstream *stderr, const char *filename, int err_code) {
    if (err_code == file_err_invalid_stat) {
        bufstream_fmt(
            stderr,
            "s S\n",
            slice_sstr("Failed to get details for file"),
            filename
        );
    } else if (err_code == file_err_failed_alloc) {
        bufstream_fmt(
            stderr,
            "s S\n",
            slice_sstr("Failed to allocate memory for file"),
            filename
        );
    } else {
        const char *err_msg = strerror(err_code);
        bufstream_fmt(
            stderr,
            "s S: S\n",
            slice_sstr("File error for file"),
            filename,
            err_msg
        );
    }
}

int file_demo(int argc, char **argv, bufstream *stdout, bufstream *stderr) {
    assert(argc > 1 && "expected at least one cli param");

    const char *filename_read = argv[1];

    file_read_result read_res = file_read_sync(filename_read, &std_allocator);
    if (read_res.err_code) {
        print_file_error(stderr, filename_read, read_res.err_code);
        return 1;
    }

    if (argc > 2) {
        const char *filename_write = argv[2];
        io_result write_res =
            file_write_sync(filename_write, read_res.data, read_res.len);
        if (write_res.err_code) {
            print_file_error(stderr, filename_write, write_res.err_code);
            return 1;
        }
    } else {
        bufstream_fmt(
            stdout,
            "s: U\ns:\ns\n",
            slice_sstr("File size"),
            read_res.len,
            slice_sstr("File contents"),
            slice_new(read_res.data, read_res.len)
        );
    }

    if (read_res.data) {
        alloc_free(&std_allocator, read_res.data);
    }
    return 0;
}

int main(int argc, char **argv) {
    uchar buf_stdout[4 * 1024];
    io_file_bytesink_context stdout_ctx = {
        .fd = STDOUT_FILENO,
        .chunk_size = 0,
    };
    bufstream bstream_stdout = {
        .buffer = buf_stdout,
        .len = 0,
        .cap = sizeof(buf_stdout),
        .sink = io_file_bytesink(&stdout_ctx),
    };
    uchar buf_stderr[4 * 1024];
    io_file_bytesink_context stderr_ctx = {
        .fd = STDERR_FILENO,
        .chunk_size = 0,
    };
    bufstream bstream_stderr = {
        .buffer = buf_stderr,
        .len = 0,
        .cap = sizeof(buf_stderr),
        .sink = io_file_bytesink(&stderr_ctx),
    };

    int ret_code = 0;
    if (argc > 1) {
        ret_code = file_demo(argc, argv, &bstream_stdout, &bstream_stderr);
    } else {
        ret_code = array_demo(&bstream_stdout);
    }

    bufstream_flush(&bstream_stdout);
    bufstream_flush(&bstream_stderr);

    return ret_code;
}
