#ifndef STRING_H_06_01_2026
#define STRING_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <stdarg.h>
////////////////////////////////////////////////////////////////////////////////

size_t str_hash(const char *);
char *str_format(char *buf, size_t len, char *fmt, ...);
int str_nprintf(char *buf, size_t bufsz, const char *fmt, ...);
int str_vnprintf(char *buf, size_t bufsz, const char *fmt, va_list args);
char *str_mem_vnprintf(char *buf, size_t bufsz, const char *fmt, va_list);

#endif
