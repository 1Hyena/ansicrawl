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
#include <stdckdint.h>
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
static inline size_t                    amp_to_ans(
    const struct amp_type *                 amp,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline size_t                    amp_row_to_ans(
    const struct amp_type *                 amp,
    uint32_t                                y,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline size_t                    amp_row_cut_to_ans(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y,
    uint32_t                                width,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline size_t                    amp_glyph_row_to_str(
    const struct amp_type *                 amp,
    uint32_t                                y,
    char *                                  str_dst,
    size_t                                  str_dst_size
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

        // not serialized:
        bool broken:1;
        bool reset:1;
    } bitset;
};

// Private API: ////////////////////////////////////////////////////////////////
static inline bool                      amp_set_style(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_style_type                   style
);
static inline ssize_t                   amp_get_cell_index(
    const struct amp_type *                 amp,
    long                                    x,
    long                                    y
);
static inline int                       amp_utf8_code_point_size(
    const char *                            utf8_str,
    size_t                                  utf8_str_size
);
static inline size_t                    amp_style_to_ans(
    struct amp_style_type                   style,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline size_t                    amp_style_update_to_ans(
    struct amp_style_type                   prev_style,
    struct amp_style_type                   next_style,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
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
static inline size_t                    amp_sub_size(
    size_t                                  a,
    size_t                                  b
);
static inline size_t                    amp_str_append(
    char *                                  str_dst,
    size_t                                  str_dst_size,
    const char *                            str_src
);
////////////////////////////////////////////////////////////////////////////////

static inline size_t amp_init(
    struct amp_type *amp, void *buf, size_t buf_size
) {
    const size_t bytes_required = (
        AMP_CELL_SIZE * amp->width * amp->height
    );
    const size_t cell_count = (
        (buf_size < bytes_required ? buf_size : bytes_required) /
        AMP_CELL_SIZE
    );
    const size_t glyph_size = cell_count * AMP_CELL_GLYPH_SIZE;
    const size_t style_size = cell_count * AMP_CELL_STYLE_SIZE;

    amp->glyph.data = (uint8_t *) buf;
    amp->glyph.size = glyph_size;

    amp->style.data = (uint8_t *) buf + amp->glyph.size;
    amp->style.size = style_size;

    amp_clear(amp);

    return bytes_required;
}

static inline void amp_clear(struct amp_type *amp) {
    memset(amp->glyph.data, 0, amp->glyph.size);
    memset(amp->style.data, 0, amp->style.size);
}

static inline ssize_t amp_get_cell_index(
    const struct amp_type *amp, long x, long y
) {
    if (x < 0 || y < 0 || x >= amp->width || y >= amp->height) {
        return -1;
    }

    return (y * amp->width + x);
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
    struct amp_type *amp, uint32_t x, uint32_t y, const char *glyph
) {
    ssize_t cell_index = amp_get_cell_index(amp, x, y);

    if (cell_index < 0) {
        return nullptr;
    }

    if ((size_t) cell_index * AMP_CELL_GLYPH_SIZE >= amp->glyph.size) {
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
        amp->glyph.data + (size_t) cell_index * AMP_CELL_GLYPH_SIZE
    );

    memcpy(dst, data, glyph_length + 1);

    return dst;
}

static inline bool amp_set_style(
    struct amp_type *amp, uint32_t x, uint32_t y,
    struct amp_style_type style
) {
    ssize_t cell_index = amp_get_cell_index(amp, x, y);

    if (cell_index < 0) {
        return false;
    }

    if ((size_t) cell_index * AMP_CELL_STYLE_SIZE >= amp->style.size) {
        return false;
    }

    return amp_style_cell_serialize(
        style, amp->style.data + (size_t) cell_index * AMP_CELL_STYLE_SIZE,
        AMP_CELL_STYLE_SIZE
    );
}

static inline const char *amp_get_glyph(
    const struct amp_type *amp, uint32_t x, uint32_t y
) {
    ssize_t cell_index = amp_get_cell_index(amp, x, y);

    if (cell_index < 0) {
        return nullptr;
    }

    if ((size_t) cell_index * AMP_CELL_GLYPH_SIZE >= amp->glyph.size) {
        return nullptr;
    }

    return (
        (const char *) amp->glyph.data +
        (size_t) cell_index * AMP_CELL_GLYPH_SIZE
    );
}

static inline struct amp_style_type amp_get_style(
    const struct amp_type *amp, uint32_t x, uint32_t y
) {
    static const struct amp_style_type broken_cell = {
        .bitset = { .broken = true }
    };

    ssize_t cell_index = amp_get_cell_index(amp, x, y);

    if (cell_index < 0) {
        return broken_cell;
    }

    if ((size_t) cell_index * AMP_CELL_STYLE_SIZE >= amp->style.size) {
        return broken_cell;
    }

    return amp_style_cell_deserialize(
        amp->style.data + (size_t) cell_index * AMP_CELL_STYLE_SIZE,
        AMP_CELL_STYLE_SIZE
    );
}

static inline size_t amp_glyph_row_to_str(
    const struct amp_type *amp, uint32_t y, char *str_dst, size_t str_dst_size
) {
    size_t str_size = 0;

    for (uint32_t x = 0, w = amp->width; x < w; ++x) {
        const char *glyph_str = amp_get_glyph(amp, x, y);

        if (!glyph_str || *glyph_str == '\0') {
            str_size += amp_str_append(
                str_dst + str_size, amp_sub_size(str_dst_size, str_size), " "
            );

            continue;
        }

        str_size += amp_str_append(
            str_dst + str_size, amp_sub_size(str_dst_size, str_size), glyph_str
        );
    }

    if (!str_size && str_dst_size) {
        *str_dst = '\0';
    }

    return (
        // The number of characters that would have been written if
        // str_dst_size had been sufficiently large, not counting the
        // terminating null character.
        str_size
    );
}

static inline size_t amp_str_append(
    char *str_dst, size_t str_dst_size, const char *str_src
) {
    int ret = snprintf(str_dst, str_dst_size, "%s", str_src);

    if (str_dst_size
    && (ret < 0 || (size_t) ret >= str_dst_size)) {
        *str_dst = '\0'; // If it did not fit, sets *str_dst to zero.
    }

    return (
        // The number of characters that would have been written if
        // str_dst_size had been sufficiently large, not counting the
        // terminating null character.
        ret >= 0 ? (size_t) ret : 0
    );
}

static inline size_t amp_style_to_ans(
    struct amp_style_type style, char *ans_dst, size_t ans_dst_size
) {
    char ans[256];
    size_t ans_size = 0;

    const char *options[] = {
        style.bitset.reset          ? "0" : nullptr,
        style.bitset.hidden         ? "8" : nullptr,
        style.bitset.faint          ? "2" : nullptr,
        style.bitset.italic         ? "3" : nullptr,
        style.bitset.underline      ? "4" : nullptr,
        style.bitset.blinking       ? "5" : nullptr,
        style.bitset.strikethrough  ? "9" : nullptr
    };

    for (size_t i=0; i<sizeof(options)/sizeof(options[0]); ++i) {
        const char *value = options[i];

        if (!value) {
            continue;
        }

        ans_size += (
            ans_size ? amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
            ) : 0
        );

        ans_size += amp_str_append(
            ans + ans_size, amp_sub_size(sizeof(ans), ans_size), value
        );
    }

    if (ans_size) {
        size_t written = 0;

        written += amp_str_append(
            ans_dst + written, amp_sub_size(ans_dst_size, written), "\x1b["
        );

        written += amp_str_append(
            ans_dst + written, amp_sub_size(ans_dst_size, written), ans
        );

        written += amp_str_append(
            ans_dst + written, amp_sub_size(ans_dst_size, written), "m"
        );

        return written;
    }
    else if (ans_dst_size) {
        *ans_dst = '\0';
    }

    return 0;
}

static inline size_t amp_style_update_to_ans(
    struct amp_style_type prev, struct amp_style_type next,
    char *ans_dst, size_t ans_dst_size
) {
    if ((prev.bitset.hidden         && !next.bitset.hidden)
    ||  (prev.bitset.faint          && !next.bitset.faint)
    ||  (prev.bitset.italic         && !next.bitset.italic)
    ||  (prev.bitset.underline      && !next.bitset.underline)
    ||  (prev.bitset.blinking       && !next.bitset.blinking)
    ||  (prev.bitset.strikethrough  && !next.bitset.strikethrough)) {
        struct amp_style_type style = next;

        style.bitset.reset = true;

        return amp_style_to_ans(style, ans_dst, ans_dst_size);
    }

    char buf[256];
    char ans[256];
    size_t ans_size = 0;

    const struct amp_style_type styles[] = {
        {
            .bitset = {
                .hidden = (
                    !prev.bitset.hidden && next.bitset.hidden
                )
            }
        },
        {
            .bitset = {
                .faint = (
                    !prev.bitset.faint && next.bitset.faint
                )
            }
        },
        {
            .bitset = {
                .italic = (
                    !prev.bitset.italic && next.bitset.italic
                )
            }
        },
        {
            .bitset = {
                .underline = (
                    !prev.bitset.underline && next.bitset.underline
                )
            }
        },
        {
            .bitset = {
                .blinking = (
                    !prev.bitset.blinking && next.bitset.blinking
                )
            }
        },
        {
            .bitset = {
                .strikethrough = (
                    !prev.bitset.strikethrough && next.bitset.strikethrough
                )
            }
        }
    };

    for (size_t i=0; i<sizeof(styles)/sizeof(styles[0]); ++i) {
        size_t buf_size = amp_style_to_ans(styles[i], buf, sizeof(buf));

        if (buf_size > 3 && buf_size < sizeof(buf)) {
            buf[buf_size - 1] = '\0'; // delete the 'm' terminator

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size), buf + 2
            );
        }
    }

    if (ans_size) {
        size_t written = 0;

        written += amp_str_append(
            ans_dst + written, amp_sub_size(ans_dst_size, written), "\x1b["
        );

        written += amp_str_append(
            ans_dst + written, amp_sub_size(ans_dst_size, written), ans
        );

        written += amp_str_append(
            ans_dst + written, amp_sub_size(ans_dst_size, written), "m"
        );

        return written;
    }
    else if (ans_dst_size) {
        *ans_dst = '\0';
    }

    return 0;
}

static inline size_t amp_row_cut_to_ans(
    const struct amp_type *amp, uint32_t x, uint32_t y, uint32_t width,
    char *ans_dst, size_t ans_dst_size
) {
    const uint32_t amp_width = amp->width;
    const uint32_t end_x = (
        width ? (x + width > amp_width ? amp_width : x + width) : amp_width
    );

    char style_ans[256];
    struct amp_style_type prev_style_state = {};
    size_t ans_size = 0;

    for (; x < end_x; ++x) {
        struct amp_style_type next_style_state = amp_get_style(amp, x, y);

        size_t style_ans_size = amp_style_update_to_ans(
            prev_style_state, next_style_state, style_ans, sizeof(style_ans)
        );

        prev_style_state = next_style_state;

        if (style_ans_size) {
            ans_size += amp_str_append(
                ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size),
                style_ans
            );
        }

        const char *glyph_str = amp_get_glyph(amp, x, y);

        if (!glyph_str || *glyph_str == '\0') {
            ans_size += amp_str_append(
                ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size), " "
            );

            continue;
        }

        ans_size += amp_str_append(
            ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size), glyph_str
        );
    }

    ans_size += amp_str_append(
        ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size), "\x1b[0m"
    );

    if (!ans_size && ans_dst_size) {
        *ans_dst = '\0';
    }

    return (
        // The number of characters that would have been written if
        // ans_dst_size had been sufficiently large, not counting the
        // terminating null character.
        ans_size
    );
}

static inline size_t amp_row_to_ans(
    const struct amp_type *amp, uint32_t y, char *ans_dst, size_t ans_dst_size
) {
    return (
        // The number of characters that would have been written if
        // ans_dst_size had been sufficiently large, not counting the
        // terminating null character.
        amp_row_cut_to_ans(amp, 0, y, amp->width, ans_dst, ans_dst_size)
    );
}

static inline size_t amp_sub_size(size_t a, size_t b) {
    size_t result;
    return ckd_sub(&result, a, b) ? 0 : result;
}

static inline size_t amp_to_ans(
    const struct amp_type *amp, char *ans_dst, size_t ans_dst_size
) {
    size_t ans_size = 0;

    for (uint32_t y = 0; y < amp->height; ++y) {
        ans_size += amp_row_to_ans(
            amp, y, ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size)
        );

        if (y + 1 < amp->height) {
            ans_size += amp_str_append(
                ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size), "\r\n"
            );
        }
    }

    if (!ans_size && ans_dst_size) {
        *ans_dst = '\0';
    }

    return ans_size;
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
            .fg             = data_size > 6 ? data[6] & (1 << 0) : 0,
            .bg             = data_size > 6 ? data[6] & (1 << 1) : 0,
            .hidden         = data_size > 6 ? data[6] & (1 << 2) : 0,
            .faint          = data_size > 6 ? data[6] & (1 << 3) : 0,
            .italic         = data_size > 6 ? data[6] & (1 << 4) : 0,
            .underline      = data_size > 6 ? data[6] & (1 << 5) : 0,
            .blinking       = data_size > 6 ? data[6] & (1 << 6) : 0,
            .strikethrough  = data_size > 6 ? data[6] & (1 << 7) : 0,
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
