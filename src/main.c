#include <stdio.h>
#include "jp.h"

int array_demo() {
    float* data = jp_dynarr_new(10, float);
    float last = 0.0;

    int i;
    for (i = 0; i < 20; i += 1) {
        float v = ((float)i)/10;
        jp_dynarr_push(data, v);
    }

    jp_dynarr_remove(data, 9);
    jp_dynarr_remove(data, 12);
    jp_dynarr_pop(data, last);

    for (i = 0; i < jp_dynarr_get_count(data); i += 1) {
        printf("data %d: %f\n", i, data[i]);
    }

    printf(
        "2: %f, 15: %f, last: %f\n",
        data[2],
        data[15],
        last
    );

    jp_dynarr_free(data);

    return 0;
}

int file_demo(int argc, char **argv) {
    assert(argc > 1 && "expected at least one cli param");

    jp_file_result result = jp_read_file(argv[1]);
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

int main(int argc, char **argv) {
    if (argc > 1) {
        return file_demo(argc, argv);
    }
    return array_demo();
}

