#include "io.h"
#include "std.h"
#include <stdio.h>

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

int file_demo(int argc, char **argv) {
    assert(argc > 1 && "expected at least one cli param");

    file_result result = read_file(argv[1], &std_allocator);
    if (result.err_code) {
        return result.err_code;
    }

    if (argc > 2) {
        write_file(argv[2], result.data, result.size);
    } else {
        printf("File size: %ld\n", result.size);
        printf("File contents:\n");
        write(1, result.data, result.size);
    }

    if (result.data) {
        alloc_free(&std_allocator, result.data);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        return file_demo(argc, argv);
    }
    return array_demo();
}
