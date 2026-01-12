#include "testr.h"
#include "io.h"
#include "std.h"
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>

static int color_enabled = 1;
static int trap_on_assert_fail = 0;
static int print_all_asserts = 0;

bool test_report_append(
    test *t,
    const bool passed,
    const char *log_message,
    const size_t log_message_size,
    const char *file,
    const int line
) {
    assert(t && "test report must not be null");
    assert(t->logs && "logs must not be null");

    test_assert assert_report = {
        .logs_offset = t->logs->len,
        .passed = passed,
        .file = file,
        .line = line,
    };
    test_assert *next_ar = dynarr_push_grow(
        t->asserts_handle->asserts, &assert_report, 1, test_assert
    );
    if (next_ar) {
        t->asserts_handle->asserts = next_ar;
    } else {
        panic();
    }

    if (!bytebuf_write_grow(
            t->logs, (const uchar *)log_message, log_message_size
        )) {
        panic();
    }

    t->assert_count += 1;
    if (passed) {
        t->asserts_passed += 1;
    }

    if (!passed && trap_on_assert_fail) {
        breakpoint();
    }
    return passed;
}

bool test_report_append_formatted_cstr(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const char *left,
    const char *right
) {
    char buffer[1024] = {0};
    cstr_fmt_result fmt_res = cstr_fmt(
        buffer, sizeof(buffer), "%s %s %s // %s", left, cmp, right, log_message
    );
    if (!fmt_res.ok) {
        panic();
    }
    buffer[fmt_res.len] = '\0';
    return test_report_append(t, passed, buffer, fmt_res.len + 1, file, line);
}

bool test_report_append_formatted_float(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const double left,
    const double right
) {
    char buffer[1024] = {0};
    cstr_fmt_result fmt_res = cstr_fmt(
        buffer, sizeof(buffer), "%f %s %f // %s", left, cmp, right, log_message
    );
    if (!fmt_res.ok) {
        panic();
    }
    buffer[fmt_res.len] = '\0';
    return test_report_append(t, passed, buffer, fmt_res.len + 1, file, line);
}

bool test_report_append_formatted_int(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    int line,
    const char *cmp,
    const llong left,
    const llong right
) {
    char buffer[1024] = {0};
    cstr_fmt_result fmt_res = cstr_fmt(
        buffer,
        sizeof(buffer),
        "%lld %s %lld // %s",
        left,
        cmp,
        right,
        log_message
    );
    if (!fmt_res.ok) {
        panic();
    }
    buffer[fmt_res.len] = '\0';
    return test_report_append(t, passed, buffer, fmt_res.len + 1, file, line);
}

bool test_report_append_formatted_uint(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    int line,
    const char *cmp,
    const ullong left,
    const ullong right
) {
    char buffer[1024] = {0};
    cstr_fmt_result fmt_res = cstr_fmt(
        buffer,
        sizeof(buffer),
        "%llu %s %llu // %s",
        left,
        cmp,
        right,
        log_message
    );
    if (!fmt_res.ok) {
        panic();
    }
    buffer[fmt_res.len] = '\0';
    return test_report_append(t, passed, buffer, fmt_res.len + 1, file, line);
}

static int test_suite_report_tap(test_suite_report *report, int fd) {
    assert(report && "test suite report must not be null");
    char txt_buf[4096];
    cstr_fmt_result fmt_res;
    io_result io_res;

    // header
    fmt_res = cstr_fmt(
        txt_buf, sizeof(txt_buf), "TAP version 14\n1..%u\n", report->test_count
    );
    if (!fmt_res.ok) {
        return -4;
    }
    io_res = io_write_all_sync(fd, txt_buf, fmt_res.len, 0);
    if (io_res.err_code) {
        return io_res.err_code;
    }

    const bytebuf *logs = report->logs;
    for (ullong i = 0; i < report->test_count; i += 1) {
        test_report tr = report->test_reports[i];

        // sub test header
        fmt_res =
            cstr_fmt(txt_buf, sizeof(txt_buf), "# Subtest: %s\n", tr.name);
        if (!fmt_res.ok) {
            return -4;
        }
        io_res = io_write_all_sync(fd, txt_buf, fmt_res.len, 0);
        if (io_res.err_code) {
            return io_res.err_code;
        }

        // nr of sub-tests
        fmt_res =
            cstr_fmt(txt_buf, sizeof(txt_buf), "    1..%u\n", tr.assert_count);
        if (!fmt_res.ok) {
            return -4;
        }
        io_res = io_write_all_sync(fd, txt_buf, fmt_res.len, 0);
        if (io_res.err_code) {
            return io_res.err_code;
        }

        // asserts as sub-tests
        for (uint j = 0; j < tr.assert_count; j += 1) {
            test_assert ar = tr.asserts[j];
            const char *status = "ok";
            if (!ar.passed) {
                status = "not ok";
            }
            fmt_res = cstr_fmt(
                txt_buf,
                sizeof(txt_buf),
                "    %s %u - %s\n",
                status,
                j + 1,
                &(logs->buffer[ar.logs_offset])
            );
            if (!fmt_res.ok) {
                return -4;
            }
            io_res = io_write_all_sync(fd, txt_buf, fmt_res.len, 0);
            if (io_res.err_code) {
                return io_res.err_code;
            }
        }

        // test status
        const char *status = "ok";
        if (tr.asserts_passed < tr.assert_count) {
            status = "not ok";
        }
        fmt_res = cstr_fmt(
            txt_buf, sizeof(txt_buf), "%s %lu - %s\n", status, i + 1, tr.name
        );
        if (!fmt_res.ok) {
            return -4;
        }
        io_res = io_write_all_sync(fd, txt_buf, fmt_res.len, 0);
        if (io_res.err_code) {
            return io_res.err_code;
        }
    }

    return 0;
}

static bool test_suite_report_pretty(test_suite_report *report, int fd) {
    assert(report && "test suite report must not be null");

    const char *color_ok = "\x1B[32m";
    const char *color_fail = "\x1B[31m";
    const char *color_skip = "\x1B[33m";
    const char *color_reset = "\x1B[0m";
    const char *color_info = "\x1B[1;30m";
    if (!color_enabled) {
        color_ok = "";
        color_fail = "";
        color_skip = "";
        color_reset = "";
        color_info = "";
    }

    io_result io_res = {0};
    if (!report->test_count) {
        io_res = io_write_str_sync(fd, "No tests executed\n");
        if (io_res.err_code) {
            return 0;
        }
        return 1;
    }

    const bytebuf *logs = report->logs;
    char msg_buffer[2048];
    cstr_fmt_result fmt_res = {0};
    for (ullong i = 0; i < report->test_count; i += 1) {
        test_report tr = report->test_reports[i];
        const char *prefix = "";
        const char *color = "";
        if (tr.assert_count == 0) {
            prefix = "SKIP";
            color = color_skip;
        } else if (tr.assert_count > tr.asserts_passed) {
            prefix = "FAIL";
            color = color_fail;
        } else {
            prefix = " OK ";
            color = color_ok;
        }

        fmt_res = cstr_fmt(
            msg_buffer,
            sizeof(msg_buffer),
            "%s[%s]%s %s %s(%u/%u passed)%s\n",
            color,
            prefix,
            color_reset,
            tr.name,
            color_info,
            tr.asserts_passed,
            tr.assert_count,
            color_reset
        );
        if (!fmt_res.ok) {
            return 0;
        }
        io_res = io_write_all_sync(fd, msg_buffer, fmt_res.len, 0);
        if (io_res.err_code) {
            return 0;
        }

        for (uint j = 0; j < tr.assert_count; j += 1) {
            test_assert ar = tr.asserts[j];
            if (!ar.passed || print_all_asserts) {
                fmt_res = cstr_fmt(
                    msg_buffer,
                    sizeof(msg_buffer),
                    "    %s:%d: %s\n",
                    ar.file,
                    ar.line,
                    &(logs->buffer[ar.logs_offset])
                );
                if (!fmt_res.ok) {
                    return 0;
                }
                io_res = io_write_all_sync(fd, msg_buffer, fmt_res.len, 0);
                if (io_res.err_code) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

int test_main(
    int argc,
    char **argv,
    test_setup *setup,
    uint test_count,
    test_case *test_cases
) {
    int ret_val = 0;

    // settings
    const char *no_color = getenv("NO_COLOR");
    if (no_color) {
        color_enabled = 0;
    } else if (!isatty(2)) {
        color_enabled = 0;
    }
    const char *assert_trap = getenv("ASSERT_TRAP");
    if (assert_trap) {
        trap_on_assert_fail = 1;
    }
    const char *assert_print_all = getenv("ASSERT_PRINT_ALL");
    if (assert_print_all) {
        print_all_asserts = 1;
    }

    // buffer to back logs
    bytebuf logs = bytebuf_new(4096, &std_allocator);

    // buffer to back assert data
    asserts_handle asserts_handle = {
        .asserts = dynarr_new(1000, test_assert, &std_allocator),
    };
    if (!asserts_handle.asserts) {
        panic();
    }

    // init report
    test_suite_report report = {
        .asserts_handle = &asserts_handle,
        .logs = &logs,
        .test_reports = alloc_new(&std_allocator, test_report, test_count),
        .test_count = test_count,
        .tests_passed = 0,
        .assert_count = 0,
        .asserts_passed = 0
    };
    if (!report.test_reports) {
        panic();
    }
    set_n(report.test_reports, 0, test_count);

    if (setup && setup->before_all) {
        setup->before_all();
    }

    for (size_t i = 0; i < test_count; i += 1) {
        test_case test_case = test_cases[i];
        test_report *tr = &report.test_reports[i];
        tr->name = test_case.name;
        tr->asserts =
            &asserts_handle.asserts[dynarr_len(asserts_handle.asserts)];

        test t = {
            .logs = &logs,
            .asserts_handle = &asserts_handle,
            .assert_count = 0,
            .asserts_passed = 0,
        };

        bool run_test = argc < 2;
        for (int i = 1; i < argc; i++) {
            if (run_test) {
                break;
            }
            if (cstr_len_unsafe(argv[i])) {
                run_test |=
                    cstr_match_wild_ascii_unsafe(test_case.name, argv[i]);
            }
        }

        if (run_test) {
            if (setup && setup->before) {
                setup->before();
            }

            test_case.test_code(&t);

            if (setup && setup->after) {
                setup->after();
            }
        }

        tr->assert_count += t.assert_count;
        tr->asserts_passed += t.asserts_passed;
        report.assert_count += t.assert_count;
        report.asserts_passed += t.asserts_passed;
        if (t.assert_count == t.asserts_passed) {
            report.tests_passed += 1;
        }
    }

    if (setup && setup->after_all) {
        setup->after_all();
    }

    int fail_count = (int)(report.test_count - report.tests_passed);
    if (fail_count > 0) {
        ret_val = 1; // Tests failed
    }

    if (!test_suite_report_pretty(&report, STDERR_FILENO)) {
        ret_val = 2; // IO error
    }
    int err_code = test_suite_report_tap(&report, STDOUT_FILENO);
    if (err_code) {
        ret_val = 2; // IO error
    }

    // free all
    alloc_free(&std_allocator, report.test_reports);
    dynarr_free(asserts_handle.asserts);
    bytebuf_free(&logs);

    return ret_val;
}
