size_t cstr_len_int(int src);
size_t cstr_len_uint(uint src);
size_t cstr_len_llong(llong src);
size_t cstr_len_ullong(ullong src);
size_t cstr_len_float(float src, uint decimals);
size_t cstr_len_double(double src, uint decimals);

size_t cstr_fmt(char *restrict dest, const char *restrict format, ...);
size_t cstr_fmt_len(char *restrict dest, const char *restrict format, ...);


