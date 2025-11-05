#include "std.h"
#include "testr.h"

// void test_ringbuf_read_write(test *t) {
//     u8 buffer[10] = {0};
//     u8 *data_p = 0;
//     slice read;
//     ringbuf rbuf;
//     ringbuf_init(&rbuf, buffer, countof(buffer));
//
//     // acquire 5
//     data_p = ringbuf_acquire(&rbuf, 5);
//     if (!assert_true(t, data_p, "first acquire must not be null")) {
//         return;
//     }
//     for (u8 i = 0; i < 5; i += 1) { data_p[i] = 100 + i; }
//
//     // acquire 5
//     data_p = ringbuf_acquire(&rbuf, 5);
//     if (!assert_true(t, data_p, "second acquire must not be null")) {
//         return;
//     }
//     for (u8 i = 0; i < 5; i += 1) { data_p[i] = 200 + i; }
//
//     // check all
//     {
//         u8 expected_data[] = {100, 101, 102, 103, 104, 200, 201, 202, 203,
//         204}; assert_eq_bytes(
//             t,
//             expected_data,
//             buffer,
//             countof(buffer),
//             "buffer must include written values"
//         );
//     }
//
//     // acquire 1
//     assert_false(t, ringbuf_acquire(&rbuf, 1), "third acquire must be null");
//
//     // read 6
//     read = ringbuf_read(&rbuf, 6);
//     {
//         u8 expected_data[] = {100, 101, 102, 103, 104, 200};
//         assert_eq_uint(
//             t, read.len, 6, "read must have expected amount of items"
//         );
//         assert_eq_bytes(
//             t,
//             expected_data,
//             read.buffer,
//             read.len,
//             "read must include written values"
//         );
//     }
//
//     // acquire 3
//     data_p = ringbuf_acquire(&rbuf, 3);
//     if (!assert_true(t, data_p, "fourth acquire must not be null")) {
//         return;
//     }
//     for (u8 i = 0; i < 3; i += 1) { data_p[i] = 10 + i; }
//
//     // check all
//     {
//         u8 expected_data[] = {10, 11, 12, 103, 104, 200, 201, 202, 203, 204};
//         assert_eq_bytes(
//             t,
//             expected_data,
//             buffer,
//             countof(buffer),
//             "buffer must include written values"
//         );
//     }
//
//     // read max
//     read = ringbuf_read(&rbuf, 100);
//     {
//         u8 expected_data[] = {201, 202, 203, 204};
//         assert_eq_uint(
//             t, read.len, 4, "read must include rest of the buffer items"
//         );
//         assert_eq_bytes(
//             t,
//             expected_data,
//             read.buffer,
//             read.len,
//             "read must include written values"
//         );
//     }
//
//     // read max
//     read = ringbuf_read(&rbuf, 100);
//     {
//         u8 expected_data[] = {10, 11, 12};
//         assert_eq_uint(
//             t,
//             read.len,
//             3,
//             "read must include everything until the write marker"
//         );
//         assert_eq_bytes(
//             t,
//             expected_data,
//             read.buffer,
//             read.len,
//             "read must include written values"
//         );
//     }
// }

static test_case tests[] = {
    // {"Ring buffer read and write", test_ringbuf_read_write},
};

setup_tests(NULL, tests)
