#include <stdio.h>
#include "jp.h"

int main(void) {
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
