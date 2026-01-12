#include "io.h"
#include "std.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int array_demo(void) {
    uchar *buffer = alloc_new(&std_allocator, uchar, 1024 * 1024);
    arena arena = arena_new(buffer, 1024 * 1024);
    allocator allocator = arena_allocator_new(&arena);

    float *arr = dynarr_new(10, float, &allocator);
    float last = 0.0;
    ullong i;

    for (i = 0; i < 20; i += 1) {
        float v = ((float)i) / 10;
        float *new_arr = dynarr_push_grow(&arr, &v, 1, float);
        if (new_arr) {
            arr = new_arr;
        } else {
            printf("allocation failed!\n");
            goto end;
        }
    }

    dynarr_remove(arr, 9);
    dynarr_remove(arr, 12);
    dynarr_pop(arr, last);

    for (i = 0; i < dynarr_len(arr); i += 1) {
        printf("data %lld: %f\n", i, arr[i]);
    }

    printf("2: %f, 15: %f, last: %f\n", arr[2], arr[15], last);

end:
    dynarr_free(arr);
    arena_clear(&arena);
    alloc_free(&std_allocator, buffer);

    return 0;
}

void print_file_error(const char *filename, int err_code) {
    if (err_code == file_err_invalid_stat) {
        printf("Failed to get details for file %s\n", filename);
    } else if (err_code == file_err_failed_alloc) {
        printf("Failed to allocate memory for file %s\n", filename);
    } else {
        const char *err_msg = strerror(err_code);
        printf("File error for file %s: %s\n", filename, err_msg);
    }
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
