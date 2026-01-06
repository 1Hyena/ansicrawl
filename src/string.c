// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <unistr.h>
#include <unistdio.h>
////////////////////////////////////////////////////////////////////////////////


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
