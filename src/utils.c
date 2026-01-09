// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
#include "all.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
////////////////////////////////////////////////////////////////////////////////


unsigned short to_ushort(long a, const char *file, int line) {
    if (a > USHRT_MAX) {
        a = USHRT_MAX;
        fuse(file, line);
    }
    else if (a < 0) {
        a = 0;
        fuse(file, line);
    }

    return (unsigned short) a;
}

size_t to_size(long a, const char *file, int line) {
    if (a < 0) {
        a = 0;
        fuse(file, line);
    }

    return (size_t) a;
}

size_t umax_size(size_t a, size_t b) {
    return a > b ? a : b;
}

bool fuse(const char *path, int line) {
    static unsigned char fuses[4096];
    char buf[128];
    const char *file = path;

    while (*path) {
        if (*path++ == '/') {
            file = path;
        }
    }

    int ret = str_nprintf(
        buf, sizeof(buf), "a fuse blows in %s on line %d", file, line
    );

    if (ret < 0) {
        BUG("str_nprintf");
        abort();
    }

    size_t hash = str_hash(buf);
    size_t byte = (hash / 8) % sizeof(fuses);
    unsigned char bit  = 1 << (hash % 8);

    if (fuses[byte] & bit) {
        return false;
    }

    fuses[byte] |= bit;
    WIZNET_BUG("%s", buf);

    return true;
}
