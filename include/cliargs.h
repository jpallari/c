#ifndef CLIARGS_H
#define CLIARGS_H

#include "std.h"

/**
 * Error code from CLI args parsing
 */
typedef enum {
    /**
     * No error
     */
    cliargs_error_none = 0,

    /**
     * Argument parsing failed
     */
    cliargs_error_parse_fail,

    /**
     * There are more flag arguments than there is space for holding all the
     * arguments.
     */
    cliargs_error_too_many_flag_args,

    /**
     * There are more positional arguments than there is space for holding all
     * the arguments.
     */
    cliargs_error_too_many_pos_args,

    /**
     * Met a flag that was not known previously.
     */
    cliargs_error_unknown_flag,

    /**
     * Flag is specified with an unknown type.
     */
    cliargs_error_unknown_type,

    /**
     * Expected a value but received another flag.
     */
    cliargs_error_value_expected,
} cliargs_error;

/**
 * Types of data expected from CLI arguments.
 */
typedef enum {
    cliargs_type_int, // signed integer (at least 64-bit wide)
    cliargs_type_uint, // unsigned integer (at least 64-bit wide)
    cliargs_type_float, // double precision floating point
    cliargs_type_str, // (null-terminated) string
    cliargs_type_bool, // boolean value (true/false)
} cliargs_type;

#define cliargs_type_int_tname "signed integer"
#define cliargs_type_uint_tname "unsigned integer"
#define cliargs_type_float_tname "real number"
#define cliargs_type_str_tname "string"
#define cliargs_type_bool_tname "boolean"

/**
 * Types of data stored from parsed CLI arguments.
 */
typedef union {
    llong sint; // signed integer (at least 64-bit wide)
    ullong uint; // unsigned integer (at least 64-bit wide)
    double real; // double precision floating point
    const char *str; // null-terminated string
} cliargs_val;

/**
 * CLI argument option specification
 */
typedef struct {
    const char *long_name;
    const char *short_name;
    const char *help_text;
    uint max_len;
    cliargs_type type;
} cliargs_opt_spec;

/**
 * CLI argument option
 */
typedef struct {
    const char *long_name;
    const char *short_name;
    const char *help_text;
    cliargs_val *vals;
    uint len;
    uint max_len;
    cliargs_type type;
} cliargs_opt;

/**
 * Set of CLI arguments
 */
typedef struct {
    /**
     * Named CLI arguments (i.e. flags)
     */
    struct {
        cliargs_opt *opts;
        uint len;
        uint max_len;
    } named;

    /**
     * Positional CLI arguments
     */
    struct {
        char const **vals;
        uint len;
        uint max_len;
    } positional;

    /**
     * Errors captured from CLI argument parsing
     */
    struct {
        char *buffer;
        size_t len;
        size_t max_len;
    } errors;
} cliargs;

/**
 * Initialize CLI args parser.
 *
 * @param args CLI args parser
 * @param named_opts_storage storage buffer for named options
 * @param named_opts_max_len maximum number of named options
 * @param pos_args_storage storage buffer for positional arguments
 * @param pos_args_max_len maximum number of positional arguments
 * @param errors_buffer string buffer for writing errors
 * @param errors_max_len maximum length of the errors buffer
 */
void cliargs_init(
    cliargs *args,
    cliargs_opt *named_opts_storage,
    uint named_opts_max_len,
    char const **pos_args_storage,
    uint pos_args_max_len,
    char *errors_buffer,
    size_t errors_max_len
);

/**
 * Add a named option to the CLI args parser.
 *
 * @param args CLI args parser
 * @param opt_spec specification for the named options
 * @param vals buffer where to store parsed arguments
 * @returns pointer that tells the count of the parsed arguments
 */
uint *cliargs_add_named(cliargs *args, cliargs_opt_spec opt_spec, void *vals);

/**
 * Parse CLI arguments from given argument list.
 *
 * @param args CLI args parser
 * @param argc number of CLI arguments provided
 * @param argv list of CLI arguments
 * @returns error code when parse fails
 */
cliargs_error cliargs_parse(cliargs *args, int argc, char **argv);

#endif
