// SPDX-License-Identifier: MIT
////////////////////////////////////////////////////////////////////////////////
// MIT License                                                                //
//                                                                            //
// Copyright (c) 2026 Erich Erstu                                             //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
////////////////////////////////////////////////////////////////////////////////

#ifndef AMP_H_12_01_2026
#define AMP_H_12_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
////////////////////////////////////////////////////////////////////////////////


struct amp_type;
struct amp_style_type;

static constexpr size_t AMP_CELL_GLYPH_SIZE = 5; // 4 bytes for UTF8 + null byte
static constexpr size_t AMP_CELL_STYLE_SIZE = 7;
static constexpr size_t AMP_CELL_SIZE       = (
    AMP_CELL_GLYPH_SIZE + AMP_CELL_STYLE_SIZE
);

// Public API: /////////////////////////////////////////////////////////////////
static inline size_t                    amp_init(
    struct amp_type *                       amp,
    void *                                  data,
    size_t                                  data_size
);
static inline void                      amp_clear(
    struct amp_type *                       amp
);
static inline size_t                    amp_row_to_str(
    const struct amp_type *                 amp,
    uint32_t                                y,
    char *                                  dst,
    size_t                                  dst_size
);
static inline size_t                    amp_glyph_row_to_str(
    const struct amp_type *                 amp,
    uint32_t                                y,
    char *                                  dst,
    size_t                                  dst_size
);
static inline const char *              amp_set_glyph(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    const char *                            glyph
);
static inline const char *              amp_get_glyph(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline struct amp_style_type     amp_get_style(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline bool                      amp_set_style(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_style_type                   style
);
////////////////////////////////////////////////////////////////////////////////

struct amp_type {
    uint32_t width;
    uint32_t height;

    struct {
        size_t size;
        uint8_t *data;
    } glyph;

    struct {
        size_t size;
        uint8_t *data;
    } style;
};

struct amp_style_type {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } fg;

    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } bg;

    struct {
        bool fg:1;
        bool bg:1;
        bool hidden:1;
        bool faint:1;
        bool italic:1;
        bool underline:1;
        bool blinking:1;
        bool strikethrough:1;
        bool broken:1;
    } bitset;
};

// Private API: ////////////////////////////////////////////////////////////////
static inline bool                      amp_set_style(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_style_type                   style
);
static ssize_t                          amp_get_cell_index(
    const struct amp_type *                 amp,
    long                                    x,
    long                                    y
);
static inline int                       amp_utf8_code_point_size(
    const char *                            str,
    size_t                                  str_sz
);
static inline size_t                    amp_style_to_str(
    struct amp_style_type                   style,
    char *                                  dst,
    size_t                                  dst_sz
);
static inline struct amp_style_type     amp_style_cell_deserialize(
    const uint8_t *                         src,
    size_t                                  src_size
);
static inline bool                      amp_style_cell_serialize(
    struct amp_style_type                   style,
    uint8_t *                               dst,
    size_t                                  dst_size
);
////////////////////////////////////////////////////////////////////////////////

static inline size_t amp_init(
    struct amp_type *img, void *buf, size_t buf_size
) {
    const size_t bytes_required = (
        AMP_CELL_SIZE * img->width * img->height
    );
    const size_t cell_count = (
        (buf_size < bytes_required ? buf_size : bytes_required) /
        AMP_CELL_SIZE
    );
    const size_t glyph_size = cell_count * AMP_CELL_GLYPH_SIZE;
    const size_t style_size = cell_count * AMP_CELL_STYLE_SIZE;

    img->glyph.data = (uint8_t *) buf;
    img->glyph.size = glyph_size;

    img->style.data = (uint8_t *) buf + img->glyph.size;
    img->style.size = style_size;

    amp_clear(img);

    return bytes_required;
}

static inline void amp_clear(struct amp_type *img) {
    memset(img->glyph.data, 0, img->glyph.size);
    memset(img->style.data, 0, img->style.size);
}

static ssize_t amp_get_cell_index(
    const struct amp_type *img, long x, long y
) {
    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return -1;
    }

    return (y * img->width + x);
}

static inline int amp_utf8_code_point_size(const char *str, size_t n) {
    uint8_t *s = (uint8_t *) str;

    if (n > 0) {
        uint8_t c = *s;

        if (c < 0x80) {
            return (c != 0 ? 1 : 0);
        }

        if (c >= 0xc2) {
            if (c < 0xe0) {
                if (n >= 2 && (s[1] ^ 0x80) < 0x40) {
                    return 2;
                }
            }
            else if (c < 0xf0) {
                if (n >= 3
                && (s[1] ^ 0x80) < 0x40
                && (s[2] ^ 0x80) < 0x40
                && (c >= 0xe1 || s[1] >= 0xa0)
                && (c != 0xed || s[1] < 0xa0)) {
                    return 3;
                }
            }
            else if (c < 0xf8) {
                if (n >= 4
                && (s[1] ^ 0x80) < 0x40
                && (s[2] ^ 0x80) < 0x40
                && (s[3] ^ 0x80) < 0x40
                && (c >= 0xf1 || s[1] >= 0x90)
                && (c < 0xf4 || (c == 0xf4 && s[1] < 0x90))) {
                    return 4;
                }
            }
        }
    }

    return -1; // invalid or incomplete multibyte character
}

static inline const char *amp_set_glyph(
    struct amp_type *img, uint32_t x, uint32_t y, const char *glyph
) {
    ssize_t cell_index = amp_get_cell_index(img, x, y);

    if (cell_index < 0) {
        return nullptr;
    }

    if ((size_t) cell_index * AMP_CELL_GLYPH_SIZE >= img->glyph.size) {
        return nullptr;
    }

    uint8_t data[AMP_CELL_GLYPH_SIZE] = {};
    uint8_t glyph_length = 0;

    for (; glyph_length < sizeof(data); ++glyph_length) {
        data[glyph_length] = (uint8_t) glyph[glyph_length];

        if (data[glyph_length] == 0) {
            break;
        }
    }

    if (glyph_length >= sizeof(data)) {
        return nullptr;
    }

    int cpsz = amp_utf8_code_point_size((const char *) data, glyph_length + 1);

    if (cpsz < 0 || cpsz > 4) {
        return nullptr;
    }

    if (cpsz < glyph_length) {
        data[cpsz] = 0;
        glyph_length = (uint8_t) cpsz;
    }

    char *dst = (char *) (
        img->glyph.data + (size_t) cell_index * AMP_CELL_GLYPH_SIZE
    );

    memcpy(dst, data, glyph_length + 1);

    return dst;
}

static inline bool amp_set_style(
    struct amp_type *img, uint32_t x, uint32_t y,
    struct amp_style_type style
) {
    ssize_t cell_index = amp_get_cell_index(img, x, y);

    if (cell_index < 0) {
        return false;
    }

    if ((size_t) cell_index * AMP_CELL_STYLE_SIZE >= img->style.size) {
        return false;
    }

    return amp_style_cell_serialize(
        style, img->style.data + (size_t) cell_index * AMP_CELL_STYLE_SIZE,
        AMP_CELL_STYLE_SIZE
    );
}

static inline const char *amp_get_glyph(
    const struct amp_type *img, uint32_t x, uint32_t y
) {
    ssize_t cell_index = amp_get_cell_index(img, x, y);

    if (cell_index < 0) {
        return nullptr;
    }

    if ((size_t) cell_index * AMP_CELL_GLYPH_SIZE >= img->glyph.size) {
        return nullptr;
    }

    return (
        (const char *) img->glyph.data +
        (size_t) cell_index * AMP_CELL_GLYPH_SIZE
    );
}

static inline struct amp_style_type amp_get_style(
    const struct amp_type *img, uint32_t x, uint32_t y
) {
    static const struct amp_style_type broken_cell = {
        .bitset = { .broken = true }
    };

    ssize_t cell_index = amp_get_cell_index(img, x, y);

    if (cell_index < 0) {
        return broken_cell;
    }

    if ((size_t) cell_index * AMP_CELL_STYLE_SIZE >= img->style.size) {
        return broken_cell;
    }

    return amp_style_cell_deserialize(
        img->style.data + (size_t) cell_index * AMP_CELL_STYLE_SIZE,
        AMP_CELL_STYLE_SIZE
    );
}

static inline size_t amp_glyph_row_to_str(
    const struct amp_type *img, uint32_t y, char *dst, size_t dst_sz
) {
    size_t str_sz = 0;

    for (uint32_t x = 0, w = img->width; x < w; ++x) {
        const char *glyph = amp_get_glyph(img, x, y);

        if (!glyph || *glyph == '\0') {
            if (str_sz + 1 < dst_sz) {
                dst[str_sz] = ' ';
            }

            ++str_sz;
            continue;
        }

        size_t gsz = strlen(glyph);

        if (str_sz + gsz < dst_sz) {
            memcpy(dst + str_sz, glyph, gsz);
        }

        str_sz += gsz;
    }

    if (str_sz < dst_sz) {
        dst[str_sz] = '\0';
    }
    else if (dst_sz) {
        dst[dst_sz - 1] = '\0';
    }

    return ++str_sz;
}

static inline size_t amp_style_to_str(
    struct amp_style_type style, char *dst, size_t dst_sz
) {
    if (style.bitset.italic) {
        int ret = snprintf(dst, dst_sz, "%s", "\x1b[3m");

        if (ret < 0 || (size_t) ret >= dst_sz) {
            return 0;
        }

        return (size_t) ret;
    }

    return 0;
}

static inline size_t amp_row_to_str(
    const struct amp_type *img, uint32_t y, char *dst, size_t dst_sz
) {
    char style_str[256];
    struct amp_style_type style_state = {};
    size_t str_sz = 0;

    for (uint32_t x = 0, w = img->width; x < w; ++x) {
        style_state = amp_get_style(img, x, y);

        size_t size = amp_style_to_str(
            style_state, style_str, sizeof(style_str)
        );

        if (size < sizeof(style_str) && str_sz + size < dst_sz) {
            memcpy(dst + str_sz, style_str, size);
        }

        str_sz += size;

        const char *glyph = amp_get_glyph(img, x, y);

        if (!glyph || *glyph == '\0') {
            if (str_sz + 1 < dst_sz) {
                dst[str_sz] = ' ';
            }

            ++str_sz;
            continue;
        }

        size_t gsz = strlen(glyph);

        if (str_sz + gsz < dst_sz) {
            memcpy(dst + str_sz, glyph, gsz);
        }

        str_sz += gsz;
    }

    int ret = snprintf(dst + str_sz, dst_sz - str_sz, "%s", "\x1b[0m");

    if (ret < 0 || (size_t) ret >= dst_sz) {
        dst[str_sz] = '\0';
    }
    else {
        str_sz += (size_t) ret;
    }

    if (str_sz < dst_sz) {
        dst[str_sz] = '\0';
    }
    else if (dst_sz) {
        dst[dst_sz - 1] = '\0';
    }

    return ++str_sz;
}

static inline struct amp_style_type amp_style_cell_deserialize(
    const uint8_t *data, size_t data_size
) {
    return (struct amp_style_type) {
        .fg = {
            .r = data_size > 0 ? data[0] : 0,
            .g = data_size > 1 ? data[1] : 0,
            .b = data_size > 2 ? data[2] : 0
        },
        .bg = {
            .r = data_size > 3 ? data[3] : 0,
            .g = data_size > 4 ? data[4] : 0,
            .b = data_size > 5 ? data[5] : 0
        },
        .bitset = {
            .fg             = data_size > 6 ? data[6] && (1 << 0) : 0,
            .bg             = data_size > 6 ? data[6] && (1 << 1) : 0,
            .hidden         = data_size > 6 ? data[6] && (1 << 2) : 0,
            .faint          = data_size > 6 ? data[6] && (1 << 3) : 0,
            .italic         = data_size > 6 ? data[6] && (1 << 4) : 0,
            .underline      = data_size > 6 ? data[6] && (1 << 5) : 0,
            .blinking       = data_size > 6 ? data[6] && (1 << 6) : 0,
            .strikethrough  = data_size > 6 ? data[6] && (1 << 7) : 0,
            .broken         = data_size < AMP_CELL_STYLE_SIZE
        }
    };
}

static inline bool amp_style_cell_serialize(
    struct amp_style_type cell, uint8_t *dst, size_t dst_size
) {
    if (dst_size < AMP_CELL_STYLE_SIZE) {
        return false;
    }

    dst[0] = cell.fg.r;
    dst[1] = cell.fg.g;
    dst[2] = cell.fg.b;
    dst[3] = cell.bg.r;
    dst[4] = cell.bg.g;
    dst[5] = cell.bg.b;
    dst[6] = (
        (cell.bitset.fg             ? (1 << 0) : 0) |
        (cell.bitset.bg             ? (1 << 1) : 0) |
        (cell.bitset.hidden         ? (1 << 2) : 0) |
        (cell.bitset.faint          ? (1 << 3) : 0) |
        (cell.bitset.italic         ? (1 << 4) : 0) |
        (cell.bitset.underline      ? (1 << 5) : 0) |
        (cell.bitset.blinking       ? (1 << 6) : 0) |
        (cell.bitset.strikethrough  ? (1 << 7) : 0)
    );

    return true;
}

#endif
