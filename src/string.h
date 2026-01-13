// SPDX-License-Identifier: MIT
#ifndef STRING_H_06_01_2026
#define STRING_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <stdarg.h>
////////////////////////////////////////////////////////////////////////////////

void str_erase(char *str, char c);
bool str_seg_to_long(const char *str, size_t str_sz, long *i);
bool str_to_long(char const *s, long *i);
size_t str_hash(const char *);
char *str_format(char *buf, size_t len, char *fmt, ...);
int str_nprintf(char *buf, size_t bufsz, const char *fmt, ...);
int str_vnprintf(char *buf, size_t bufsz, const char *fmt, va_list args);
char *str_mem_vnprintf(char *buf, size_t bufsz, const char *fmt, va_list);
const char *str_seg_skip_utf8_symbol(const char *str, size_t str_sz);
const char *str_seg_skip_digits(const char *str, size_t str_sz);

char *str_format(
    char *buf, size_t len, char *fmt, ...
) __attribute__((format (printf, 3, 4)));

#endif
