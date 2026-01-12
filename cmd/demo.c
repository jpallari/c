#include "io.h"
#include "std.h"
#include <string.h>
#include <unistd.h>

int array_demo(void) {
    int exit_code = 0;
    char txt_buf[1024] = {0};
    cstr_fmt_result fmt_res = {0};
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
        fmt_res =
            cstr_fmt(txt_buf, sizeof(txt_buf), "data %llu: %f\n", i, arr[i]);
        if (!fmt_res.ok) {
            exit_code = 1;
            goto end;
        }
        io_write_all_sync(
            STDOUT_FILENO, txt_buf, cstr_fmt_result_non_null_len(&fmt_res), 0
        );
    }

    fmt_res = cstr_fmt(
        txt_buf,
        sizeof(txt_buf),
        "2: %f, 15: %f, last: %f\n",
        arr[2],
        arr[15],
        last
    );
    if (!fmt_res.ok) {
        exit_code = 1;
        goto end;
    }
    io_write_all_sync(
        STDOUT_FILENO, txt_buf, cstr_fmt_result_non_null_len(&fmt_res), 0
    );

end:
    dynarr_free(arr);
    arena_clear(&arena);
    alloc_free(&std_allocator, buffer);

    return exit_code;
}

void print_file_error(const char *filename, int err_code) {
    char txt_buf[1024];
    cstr_fmt_result fmt_res;
    if (err_code == file_err_invalid_stat) {
        fmt_res = cstr_fmt(
            txt_buf,
            sizeof(txt_buf),
            "Failed to get details for file %s\n",
            filename
        );
    } else if (err_code == file_err_failed_alloc) {
        fmt_res = cstr_fmt(
            txt_buf,
            sizeof(txt_buf),
            "Failed to allocate memory for file %s\n",
            filename
        );
    } else {
        const char *err_msg = strerror(err_code);
        fmt_res = cstr_fmt(
            txt_buf,
            sizeof(txt_buf),
            "File error for file %s: %s\n",
            filename,
            err_msg
        );
    }
    io_write_all_sync(
        STDERR_FILENO, txt_buf, cstr_fmt_result_non_null_len(&fmt_res), 0
    );
}

int file_demo(int argc, char **argv) {
    assert(argc > 1 && "expected at least one cli param");

    const int fd = STDOUT_FILENO;
    const char *filename_read = argv[1];

    file_read_result read_res = file_read_sync(filename_read, &std_allocator);
    if (read_res.err_code) {
        print_file_error(filename_read, read_res.err_code);
        return 1;
    }

    if (argc > 2) {
        const char *filename_write = argv[2];
        io_result write_res =
            file_write_sync(filename_write, read_res.data, read_res.len);
        if (write_res.err_code) {
            print_file_error(filename_write, write_res.err_code);
            return 1;
        }
    } else {
        char buffer[1024];
        cstr_fmt_result fmt_res =
            cstr_fmt(buffer, sizeof(buffer), "File size: %lu\n", read_res.len);
        io_write_all_sync(fd, buffer, fmt_res.len, 0);
        io_write_str_sync(fd, "File contents: \n");
        io_write_all_sync(fd, read_res.data, read_res.len, 0);
    }

    if (read_res.data) {
        alloc_free(&std_allocator, read_res.data);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        return file_demo(argc, argv);
    }
    return array_demo();
}
