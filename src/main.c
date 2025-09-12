#include <stdio.h>
#include "jp.h"

int main(void) {
    float* data = jp_dynarr_new(10, float);

    int i;
    for (i = 0; i < 20; i += 1) {
        float v = ((float)i)/10;
        jp_dynarr_push(data, v);
    }

    for (i = 0; i < jp_dynarr_get_count(data); i += 1) {
        printf("data %d: %f\n", i, data[i]);
    }

    jp_dynarr_free(data);
    return 0;
}
