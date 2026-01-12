#include "io.h"
#include "std.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        io_stderr_write_sstr("No file supplied\n");
        return 1;
    }

    int mode = 1;
    if (argc > 2 && cstr_eq_unsafe(argv[2], "float")) {
        mode = 2;
    }

    file_read_result res = file_read_sync(argv[1], &std_allocator);
    if (res.err_code) {
        io_stderr_fmt(
            "S 'S' S i",
            "Reading file",
            argv[1],
            "failed with code\n",
            res.err_code
        );
    }

    cstr_split_iter split = {
        .str = (char *)res.data,
        .str_len = res.len,
        .split_chars = "\n",
        .split_chars_len = 1,
        .null_terminate = 1,
        .index = 0,
    };

    slice s = {0};
    do {
        s = cstr_split_next(&split);
        if (!s.buffer) {
            break;
        }

        char buf[1000] = {0};
        double d = 0.0;
        float f = 0.0;
        bool ok = 0;

        if (mode == 2) {
            ok = cstr_to_float((char *)s.buffer, s.len, &f) > 0;
        } else {
            ok = cstr_to_double((char *)s.buffer, s.len, &d) > 0;
        }

        if (ok && mode == 2) {
            size_t len = cstr_from_float(buf, sizeof(buf), f, 6);
            if (len) {
                printf("%s %g %s\n", s.buffer, f, buf);
            } else {
                printf("%s %g ERR\n", s.buffer, f);
            }
        } else if (ok) {
            size_t len = cstr_from_double(buf, sizeof(buf), d, 18);
            if (len) {
                printf("%s %g %s\n", s.buffer, d, buf);
            } else {
                printf("%s %g ERR\n", s.buffer, f);
            }
        } else {
            printf("fail: %s\n", s.buffer);
        }
    } while (s.buffer);

    alloc_free(&std_allocator, res.data);
    io_stderr_flush();

    return 0;
}
