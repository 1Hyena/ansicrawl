// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <unistr.h>
#include <unistdio.h>
#include <errno.h>
////////////////////////////////////////////////////////////////////////////////


bool str_seg_to_long(const char *str, size_t str_sz, long *result) {
    char buf[64] = {};
    size_t i = 0;
    size_t j = 0;

    for (; i + 1 < ARRAY_LENGTH(buf) && j < str_sz; ++j) {
        if (i == 0 && str[j] == ' ') {
            continue;
        }

        buf[i++] = str[j];
    }

    return str_to_long(buf, result);
}

bool str_to_long(char const *s, long *i) {
    char *end;
    long long l;

    errno = 0;
    l = strtoll(s, &end, 10);

    if (errno == ERANGE && l == LLONG_MAX) {
        return false;
    }

    if (errno == ERANGE && l == LLONG_MIN) {
        return false;
    }

    if (*s == '\0' || *end != '\0') {
        return false;
    }

    if (l > LONG_MAX || l < LONG_MIN) {
        return false;
    }

    *i = (long) l;

    return true;
}

const char *str_seg_skip_utf8_symbol(const char *str, size_t str_sz) {
    if (!str_sz) {
        return str;
    }

    int sz = u8_mblen((const uint8_t *) str, str_sz);

    if (sz == 0) {
        return str;
    }
    else if (sz < 0) {
        return str + 1;
    }

    return str + sz;
}

const char *str_seg_skip_digits(const char *str, size_t str_sz) {
    const char *s = str;

    while (*s && s < str + str_sz) {
        const char *next = str_seg_skip_utf8_symbol(
            s, str_sz - SIZEVAL(s - str)
        );

        if (next > str + str_sz ) {
            FUSE(); // Regression guard, should never trigger.
            break;
        }

        if (next == s) {
            break;
        }

        --next;

        switch (*next) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                break;
            }
            default: {
                return s;
            }
        }

        s = next + 1;
    }

    return s;
}

size_t str_hash(const char *str) {
    size_t hash = 5381;
    unsigned char c = 0;

    while ((c = (unsigned char) *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

char *str_format(char *buf, size_t len, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = str_vnprintf(buf, len, fmt, args);
    va_end(args);

    return (ret >= 0 && SIZEVAL(ret) < len) ? buf + ret : nullptr;
}

int str_nprintf(char *buf, size_t bufsz, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = str_vnprintf(buf, bufsz, fmt, args);
    va_end(args);
    return ret;
}

int str_vnprintf(char *buf, size_t bufsz, const char *fmt, va_list args) {
    return u8_u8_vsnprintf((uint8_t *) buf, bufsz, (const uint8_t *) fmt, args);
}

char *str_mem_vnprintf(char *buf, size_t bufsz, const char *fmt, va_list args) {
    if (bufsz) {
        if (buf == nullptr) {
            FUSE();

            return nullptr;
        }
        else {
            *buf = '\0';
        }
    }

    char *bufptr = buf;

    va_list dup_args;
    va_copy(dup_args, args);

    for (size_t i=0; i<2; ++i) {
        int cx = str_vnprintf(bufptr, bufsz, fmt, dup_args);

        va_end(dup_args);

        if ((cx >= 0 && (size_t) cx < bufsz) || cx < 0) {
            break;
        }

        if (bufptr == buf) {
            if (bufsz && buf) {
                *buf = '\0';
            }

            bufsz = SIZEVAL(cx) + 1;

            MEM *mem = mem_new(
                alignof(typeof(*bufptr)), sizeof(*bufptr) * bufsz
            );

            bufptr = mem ? mem->data : nullptr;

            if (!bufptr) {
                FUSE();
                return nullptr;
            }

            va_copy(dup_args, args);
        }
        else {
            break;
        }
    }

    return bufptr;
}
