#include "jp.h"
#include <stdio.h>

int array_demo(void) {
    u8 *buffer = jp_new(u8, 1024 * 1024, &jp_std_allocator);
    jp_arena arena = jp_arena_new(buffer, 1024 * 1024);
    jp_allocator allocator = jp_arena_allocator_new(&arena);

    float *arr = jp_dynarr_new(10, float, &allocator);
    float last = 0.0;
    int i;

    for (i = 0; i < 20; i += 1) {
        float v = ((float)i) / 10;
        float *new_arr = jp_dynarr_push_grow(&arr, &v, 1, float);
        if (new_arr) {
            arr = new_arr;
        } else {
            printf("allocation failed!\n");
            goto end;
        }
    }

    jp_dynarr_remove(arr, 9);
    jp_dynarr_remove(arr, 12);
    jp_dynarr_pop(arr, last);

    for (i = 0; i < jp_dynarr_len(arr); i += 1) {
        printf("data %d: %f\n", i, arr[i]);
    }

    printf("2: %f, 15: %f, last: %f\n", arr[2], arr[15], last);

end:
    jp_dynarr_free(arr);
    jp_arena_clear(&arena);
    jp_free(buffer, &jp_std_allocator);

    return 0;
}

int file_demo(int argc, char **argv) {
    assert(argc > 1 && "expected at least one cli param");

    jp_file_result result = jp_read_file(argv[1], &jp_std_allocator);
    if (result.err_code) {
        return result.err_code;
    }

    if (argc > 2) {
        jp_write_file(argv[2], result.data, result.size);
    } else {
        printf("File size: %ld\n", result.size);
        printf("File contents:\n");
        write(1, result.data, result.size);
    }

    if (result.data) {
        free(result.data);
    }
    return 0;
}
