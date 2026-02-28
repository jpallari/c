#include "testr.h"
#include "io.h"
#include "std.h"
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>

static int color_enabled = 1;
static int print_all_asserts = 0;

static void test_report_append_no_log(
    test *t,
    const bool passed,
    const size_t logs_offset,
    const size_t logs_len,
    const char *file,
    const int line
) {
    assert(t && "test report must not be null");
    test_assert assert_report = {
        .logs_offset = logs_offset,
        .logs_len = logs_len,
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

    t->assert_count += 1;
    if (passed) {
        t->asserts_passed += 1;
    }
}

bool test_report_append(
    test *t,
    const bool passed,
    const char *log_message,
    const size_t log_message_size,
    const char *file,
    const int line
) {
    assert(t->logs && "logs must not be null");
    bytebuf_result res =
        bytebuf_write_str(t->logs, log_message, log_message_size);
    if (!res.ok) {
        panic();
    }
    test_report_append_no_log(t, passed, res.offset, res.len, file, line);
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
    assert(t->logs && "logs must not be null");
    bytebuf_result res =
        bytebuf_fmt(t->logs, "S S S // S", left, cmp, right, log_message);
    if (!res.ok) {
        panic();
    }
    test_report_append_no_log(t, passed, res.offset, res.len, file, line);
    return passed;
}

bool test_report_append_formatted_hex(
    test *t,
    bool passed,
    const char *log_message,
    const char *file,
    const int line,
    const char *cmp,
    const slice_const left,
    const slice_const right
) {
    assert(t->logs && "logs must not be null");
    bytebuf_result res =
        bytebuf_fmt(t->logs, "0xh S 0xh // S", left, cmp, right, log_message);
    if (!res.ok) {
        panic();
    }
    test_report_append_no_log(t, passed, res.offset, res.len, file, line);
    return passed;
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
    assert(t->logs && "logs must not be null");
    bytebuf_result res =
        bytebuf_fmt(t->logs, "f S f // S", left, cmp, right, log_message);
    if (!res.ok) {
        panic();
    }
    test_report_append_no_log(t, passed, res.offset, res.len, file, line);
    return passed;
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
    assert(t->logs && "logs must not be null");
    bytebuf_result res =
        bytebuf_fmt(t->logs, "I S I // S", left, cmp, right, log_message);
    if (!res.ok) {
        panic();
    }
    test_report_append_no_log(t, passed, res.offset, res.len, file, line);
    return passed;
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
    assert(t->logs && "logs must not be null");
    bytebuf_result res =
        bytebuf_fmt(t->logs, "U S U // S", left, cmp, right, log_message);
    if (!res.ok) {
        panic();
    }
    test_report_append_no_log(t, passed, res.offset, res.len, file, line);
    return passed;
}

static int test_suite_report_tap(test_suite_report *report, bufstream *out) {
    assert(report && "test suite report must not be null");
    bufstream_write_result out_res;

    // header
    out_res = bufstream_fmt(
        out, "s\n1..u\n", slice_sstr("TAP version 14"), report->test_count
    );
    if (out_res.err_code) {
        return out_res.err_code;
    }

    const bytebuf *logs = report->logs;
    for (ullong i = 0; i < report->test_count; i += 1) {
        test_report tr = report->test_reports[i];

        // sub test header
        out_res =
            bufstream_fmt(out, "# s: S\n", slice_sstr("Subtest"), tr.name);
        if (out_res.err_code) {
            return out_res.err_code;
        }

        // nr of sub-tests
        out_res = bufstream_fmt(out, "    1..u\n", tr.assert_count);
        if (out_res.err_code) {
            return out_res.err_code;
        }

        // asserts as sub-tests
        for (uint j = 0; j < tr.assert_count; j += 1) {
            test_assert ar = tr.asserts[j];
            slice_const status = slice_sstr("ok");
            if (!ar.passed) {
                status = slice_sstr("not ok");
            }
            out_res = bufstream_fmt(
                out,
                "    s u - s\n",
                status,
                j + 1,
                slice_new((logs->buffer + ar.logs_offset), ar.logs_len)
            );
            if (out_res.err_code) {
                return out_res.err_code;
            }
        }

        // test status
        slice_const status = slice_sstr("ok");
        if (tr.asserts_passed < tr.assert_count) {
            status = slice_sstr("not ok");
        }
        out_res = bufstream_fmt(out, "s U - S\n", status, i + 1, tr.name);
        if (out_res.err_code) {
            return out_res.err_code;
        }
    }

    return 0;
}

static bool
test_suite_report_pretty(test_suite_report *report, bufstream *out) {
    assert(report && "test suite report must not be null");

    slice_const color_ok;
    slice_const color_fail;
    slice_const color_skip;
    slice_const color_reset;
    slice_const color_info;
    if (color_enabled) {
        color_ok = slice_sstr("\x1B[32m");
        color_fail = slice_sstr("\x1B[31m");
        color_skip = slice_sstr("\x1B[33m");
        color_reset = slice_sstr("\x1B[0m");
        color_info = slice_sstr("\x1B[1;30m");
    } else {
        color_ok = slice_sstr("");
        color_fail = slice_sstr("");
        color_skip = slice_sstr("");
        color_reset = slice_sstr("");
        color_info = slice_sstr("");
    }

    bufstream_write_result io_res = {0};
    if (!report->test_count) {
        io_res = bufstream_write_sstr(out, "No tests executed\n");
        if (io_res.err_code) {
            return 0;
        }
        return 1;
    }

    const bytebuf *logs = report->logs;
    for (ullong i = 0; i < report->test_count; i += 1) {
        test_report tr = report->test_reports[i];
        slice_const prefix;
        slice_const color;
        if (tr.assert_count == 0) {
            prefix = slice_sstr("SKIP");
            color = color_skip;
        } else if (tr.assert_count > tr.asserts_passed) {
            prefix = slice_sstr("FAIL");
            color = color_fail;
        } else {
            prefix = slice_sstr(" OK ");
            color = color_ok;
        }

        io_res = bufstream_fmt(
            out,
            "s[s]s S s(u/u s)s\n",
            color,
            prefix,
            color_reset,
            tr.name,
            color_info,
            tr.asserts_passed,
            tr.assert_count,
            slice_sstr("passed"),
            color_reset
        );
        if (io_res.err_code) {
            return 0;
        }

        for (uint j = 0; j < tr.assert_count; j += 1) {
            test_assert ar = tr.asserts[j];
            if (!ar.passed || print_all_asserts) {
                io_res = bufstream_fmt(
                    out,
                    "    S:i: s\n",
                    ar.file,
                    ar.line,
                    slice_new((logs->buffer + ar.logs_offset), ar.logs_len)
                );
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
    allocation test_reports_allocation =
        alloc_new(&std_allocator, test_report, test_count);
    test_suite_report report = {
        .asserts_handle = &asserts_handle,
        .logs = &logs,
        .test_reports = test_reports_allocation.ptr,
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
            if (cstr_byte_len_unsafe(argv[i])) {
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

        bytesink_result io_res = io_stderr_flush();
        if (io_res.err_code) {
            ret_val = 2;
            break;
        }
        io_res = io_stdout_flush();
        if (io_res.err_code) {
            ret_val = 2;
            break;
        }
    }

    if (setup && setup->after_all) {
        setup->after_all();
    }

    int fail_count = (int)(report.test_count - report.tests_passed);
    if (fail_count > 0) {
        ret_val = 1; // Tests failed
    }

    if (!test_suite_report_pretty(&report, io_stderr_get())) {
        ret_val = 2; // IO error
    }
    int err_code = test_suite_report_tap(&report, io_stdout_get());
    if (err_code) {
        ret_val = 2; // IO error
    }

    // free all
    alloc_free(&std_allocator, test_reports_allocation);
    dynarr_free(asserts_handle.asserts);
    bytebuf_free(&logs);

    bytesink_result io_res = io_stderr_flush();
    if (io_res.err_code) {
        ret_val = 2;
    }
    io_res = io_stdout_flush();
    if (io_res.err_code) {
        ret_val = 2;
    }

    return ret_val;
}
