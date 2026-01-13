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
#include <limits.h>
////////////////////////////////////////////////////////////////////////////////


struct amp_type;
struct amp_rgb_type;
struct amp_style_type;

static constexpr size_t AMP_CELL_GLYPH_SIZE = 5; // 4 bytes for UTF8 + null byte
static constexpr size_t AMP_CELL_STYLE_SIZE = 7;
static constexpr size_t AMP_CELL_SIZE       = (
    AMP_CELL_GLYPH_SIZE + AMP_CELL_STYLE_SIZE
);

typedef enum : uint8_t {
    AMP_COLOR_NONE = 0,
    ////////////////////////////////////////////////////////////////////////////
    AMP_BLACK,      AMP_MAROON,     AMP_GREEN,      AMP_OLIVE,      AMP_NAVY,
    AMP_PURPLE,     AMP_TEAL,       AMP_SILVER,     AMP_GRAY,       AMP_RED,
    AMP_LIME,       AMP_YELLOW,     AMP_BLUE,       AMP_MAGENTA,    AMP_CYAN,
    AMP_WHITE,
    ////////////////////////////////////////////////////////////////////////////
    AMP_MAX_COLOR
} AMP_COLOR;

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
static inline bool                      amp_set_style(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_style_type                   style
);
static inline bool                      amp_set_background(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_rgb_type                     rgb
);
static inline bool                      amp_reset_background(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline bool                      amp_set_foreground(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_rgb_type                     rgb
);
static inline bool                      amp_reset_foreground(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline struct amp_style_type     amp_get_style(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline struct amp_rgb_type       amp_rgb(
    uint8_t                                 r,
    uint8_t                                 g,
    uint8_t                                 b
);
static inline struct amp_rgb_type       amp_color_rgb(
    AMP_COLOR                               color
);
////////////////////////////////////////////////////////////////////////////////

typedef enum : uint8_t {
    AMP_PAL_RGB16 = 0,
    AMP_PAL_24BIT
} AMP_PALETTE;

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

    AMP_PALETTE palette;
};

struct amp_rgb_type {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct amp_style_type {
    struct amp_rgb_type fg;
    struct amp_rgb_type bg;

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

static const char *amp_number_table[] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13",
    "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25",
    "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37",
    "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61",
    "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73",
    "74", "75", "76", "77", "78", "79", "80", "81", "82", "83", "84", "85",
    "86", "87", "88", "89", "90", "91", "92", "93", "94", "95", "96", "97",
    "98", "99", "100", "101", "102", "103", "104", "105", "106", "107", "108",
    "109", "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
    "120", "121", "122", "123", "124", "125", "126", "127", "128", "129", "130",
    "131", "132", "133", "134", "135", "136", "137", "138", "139", "140", "141",
    "142", "143", "144", "145", "146", "147", "148", "149", "150", "151", "152",
    "153", "154", "155", "156", "157", "158", "159", "160", "161", "162", "163",
    "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174",
    "175", "176", "177", "178", "179", "180", "181", "182", "183", "184", "185",
    "186", "187", "188", "189", "190", "191", "192", "193", "194", "195", "196",
    "197", "198", "199", "200", "201", "202", "203", "204", "205", "206", "207",
    "208", "209", "210", "211", "212", "213", "214", "215", "216", "217", "218",
    "219", "220", "221", "222", "223", "224", "225", "226", "227", "228", "229",
    "230", "231", "232", "233", "234", "235", "236", "237", "238", "239", "240",
    "241", "242", "243", "244", "245", "246", "247", "248", "249", "250", "251",
    "252", "253", "254", "255"
};

static const struct amp_rgb16_type {
    const char *        code;
    struct amp_rgb_type rgb;
    AMP_COLOR           index;
    bool                bright:1;
} amp_rgb16_fg_table[] = {
    [AMP_COLOR_NONE] = {
        .index  = AMP_COLOR_NONE,
        .code   = ""
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_BLACK] = {
        .index  = AMP_BLACK,
        .code   = "30",
        .rgb    = { .r  = 0,    .g  = 0,    .b  = 0     }
    },
    [AMP_MAROON] = {
        .index  = AMP_MAROON,
        .code   = "31",
        .rgb    = { .r  = 128,  .g  = 0,    .b  = 0     }
    },
    [AMP_GREEN] = {
        .index  = AMP_GREEN,
        .code   = "32",
        .rgb    = { .r  = 0,    .g  = 128,  .b  = 0     }
    },
    [AMP_OLIVE] = {
        .index  = AMP_OLIVE,
        .code   = "33",
        .rgb    = { .r  = 128,  .g  = 128,  .b  = 0     }
    },
    [AMP_NAVY] = {
        .index  = AMP_NAVY,
        .code   = "34",
        .rgb    = { .r  = 0,    .g  = 0,    .b  = 128   }
    },
    [AMP_PURPLE] = {
        .index  = AMP_PURPLE,
        .code   = "35",
        .rgb    = { .r  = 128,  .g  = 0,    .b  = 128   }
    },
    [AMP_TEAL] = {
        .index  = AMP_TEAL,
        .code   = "36",
        .rgb    = { .r  = 0,    .g  = 128,  .b  = 128   }
    },
    [AMP_SILVER] = {
        .index  = AMP_SILVER,
        .code   = "37",
        .rgb    = { .r  = 128,  .g  = 128,  .b  = 128   }
    },
    [AMP_GRAY] = {
        .index  = AMP_GRAY,
        .code   = "30",
        .rgb    = { .r  = 64,   .g  = 64,   .b  = 64    },
        .bright = true
    },
    [AMP_RED] = {
        .index  = AMP_RED,
        .code   = "31",
        .rgb    = { .r  = 255,  .g  = 0,    .b  = 0     },
        .bright = true
    },
    [AMP_LIME] = {
        .index  = AMP_LIME,
        .code   = "32",
        .rgb    = { .r  = 0,    .g  = 255,  .b  = 0     },
        .bright = true
    },
    [AMP_YELLOW] = {
        .index  = AMP_YELLOW,
        .code   = "33",
        .rgb    = { .r  = 255,  .g  = 255,  .b  = 0     },
        .bright = true
    },
    [AMP_BLUE] = {
        .index  = AMP_BLUE,
        .code   = "34",
        .rgb    = { .r  = 0,    .g  = 0,    .b  = 255   },
        .bright = true
    },
    [AMP_MAGENTA] = {
        .index  = AMP_MAGENTA,
        .code   = "35",
        .rgb    = { .r  = 255,  .g  = 0,    .b  = 255   },
        .bright = true
    },
    [AMP_CYAN] = {
        .index  = AMP_CYAN,
        .code   = "36",
        .rgb    = { .r  = 0,    .g  = 255,  .b  = 255   },
        .bright = true
    },
    [AMP_WHITE] = {
        .index  = AMP_WHITE,
        .code   = "37",
        .rgb    = { .r  = 255,  .g  = 255,  .b  = 255   },
        .bright = true
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_MAX_COLOR] = {}
};

static const struct amp_rgb16_type amp_rgb16_bg_table[] = {
    [AMP_COLOR_NONE] = {
        .index  = AMP_COLOR_NONE,
        .code   = ""
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_BLACK] = {
        .index  = AMP_BLACK,
        .code   = "40",
        .rgb    = { .r  = 0,    .g  = 0,    .b  = 0     }
    },
    [AMP_MAROON] = {
        .index  = AMP_MAROON,
        .code   = "41",
        .rgb    = { .r  = 128,  .g  = 0,    .b  = 0     }
    },
    [AMP_GREEN] = {
        .index  = AMP_GREEN,
        .code   = "42",
        .rgb    = { .r  = 0,    .g  = 128,  .b  = 0     }
    },
    [AMP_OLIVE] = {
        .index  = AMP_OLIVE,
        .code   = "43",
        .rgb    = { .r  = 128,  .g  = 128,  .b  = 0     }
    },
    [AMP_NAVY] = {
        .index  = AMP_NAVY,
        .code   = "44",
        .rgb    = { .r  = 0,    .g  = 0,    .b  = 128   }
    },
    [AMP_PURPLE] = {
        .index  = AMP_PURPLE,
        .code   = "45",
        .rgb    = { .r  = 128,  .g  = 0,    .b  = 128   }
    },
    [AMP_TEAL] = {
        .index  = AMP_TEAL,
        .code   = "46",
        .rgb    = { .r  = 0,    .g  = 128,  .b  = 128   }
    },
    [AMP_SILVER] = {
        .index  = AMP_SILVER,
        .code   = "47",
        .rgb    = { .r  = 128,  .g  = 128,  .b  = 128   }
    },
    [AMP_GRAY] = {
        .index  = AMP_GRAY,
        .code   = "40",
        .rgb    = { .r  = 64,   .g  = 64,   .b  = 64    },
        .bright = true
    },
    [AMP_RED] = {
        .index  = AMP_RED,
        .code   = "41",
        .rgb    = { .r  = 255,  .g  = 0,    .b  = 0     },
        .bright = true
    },
    [AMP_LIME] = {
        .index  = AMP_LIME,
        .code   = "42",
        .rgb    = { .r  = 0,    .g  = 255,  .b  = 0     },
        .bright = true
    },
    [AMP_YELLOW] = {
        .index  = AMP_YELLOW,
        .code   = "43",
        .rgb    = { .r  = 255,  .g  = 255,  .b  = 0     },
        .bright = true
    },
    [AMP_BLUE] = {
        .index  = AMP_BLUE,
        .code   = "44",
        .rgb    = { .r  = 0,    .g  = 0,    .b  = 255   },
        .bright = true
    },
    [AMP_MAGENTA] = {
        .index  = AMP_MAGENTA,
        .code   = "45",
        .rgb    = { .r  = 255,  .g  = 0,    .b  = 255   },
        .bright = true
    },
    [AMP_CYAN] = {
        .index  = AMP_CYAN,
        .code   = "46",
        .rgb    = { .r  = 0,    .g  = 255,  .b  = 255   },
        .bright = true
    },
    [AMP_WHITE] = {
        .index  = AMP_WHITE,
        .code   = "47",
        .rgb    = { .r  = 255,  .g  = 255,  .b  = 255   },
        .bright = true
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_MAX_COLOR] = {}
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
    AMP_PALETTE                             palette,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline size_t                    amp_style_update_to_ans(
    struct amp_style_type                   prev_style,
    struct amp_style_type                   next_style,
    AMP_PALETTE                             palette,
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
static inline struct amp_rgb16_type     amp_find_rgb16(
    const struct amp_rgb16_type *           table,
    struct amp_rgb_type                     rgb
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

static inline bool amp_set_background(
    struct amp_type *amp, uint32_t x, uint32_t y, struct amp_rgb_type rgb
) {
    struct amp_style_type style = amp_get_style(amp, x, y);

    style.bg.r = rgb.r;
    style.bg.g = rgb.g;
    style.bg.b = rgb.b;
    style.bitset.bg = true;

    return amp_set_style(amp, x, y, style);
}

static inline bool amp_reset_background(
    struct amp_type *amp, uint32_t x, uint32_t y
) {
    struct amp_style_type style = amp_get_style(amp, x, y);

    style.bitset.bg = false;

    return amp_set_style(amp, x, y, style);
}

static inline bool amp_set_foreground(
    struct amp_type *amp, uint32_t x, uint32_t y, struct amp_rgb_type rgb
) {
    struct amp_style_type style = amp_get_style(amp, x, y);

    style.fg.r = rgb.r;
    style.fg.g = rgb.g;
    style.fg.b = rgb.b;
    style.bitset.fg = true;

    return amp_set_style(amp, x, y, style);
}

static inline bool amp_reset_foreground(
    struct amp_type *amp, uint32_t x, uint32_t y
) {
    struct amp_style_type style = amp_get_style(amp, x, y);

    style.bitset.fg = false;

    return amp_set_style(amp, x, y, style);
}

static inline struct amp_rgb_type amp_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (struct amp_rgb_type) { .r = r, .g = g, .b = b };
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
    struct amp_style_type style, AMP_PALETTE pal,
    char *ans_dst, size_t ans_dst_size
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

    if (pal == AMP_PAL_24BIT) {
        if (style.bitset.fg) {
            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size), "38;2;"
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[style.fg.r]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[style.fg.g]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[style.fg.b]
            );
        }

        if (style.bitset.bg) {
            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size), "48;2;"
            );


            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[style.bg.r]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[style.bg.g]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[style.bg.b]
            );
        }
    }
    else {
        if (style.bitset.bg) {
            auto bg_rgb_row = amp_find_rgb16(
                amp_rgb16_bg_table, (struct amp_rgb_type) {
                    .r = style.bg.r,
                    .g = style.bg.g,
                    .b = style.bg.b
                }
            );

            if (bg_rgb_row.bright) {
                struct amp_rgb_type buf = style.bg;
                style.bg = style.fg;
                style.fg = buf;
                style.bitset.bg = style.bitset.fg;
                style.bitset.fg = true;

                ans_size += (
                    ans_size ? amp_str_append(
                        ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                    ) : 0
                );

                ans_size += amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), "7"
                );
            }
        }

        if (style.bitset.fg) {
            auto fg_rgb_row = amp_find_rgb16(
                amp_rgb16_fg_table, (struct amp_rgb_type) {
                    .r = style.fg.r,
                    .g = style.fg.g,
                    .b = style.fg.b
                }
            );

            if (fg_rgb_row.bright) {
                ans_size += (
                    ans_size ? amp_str_append(
                        ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                    ) : 0
                );

                ans_size += amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), "1"
                );
            }

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                fg_rgb_row.code
            );
        }

        if (style.bitset.bg) {
            auto bg_rgb_row = amp_find_rgb16(
                amp_rgb16_bg_table, (struct amp_rgb_type) {
                    .r = style.bg.r,
                    .g = style.bg.g,
                    .b = style.bg.b
                }
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                bg_rgb_row.code
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

static inline size_t amp_style_update_to_ans(
    struct amp_style_type prev, struct amp_style_type next, AMP_PALETTE pal,
    char *ans_dst, size_t ans_dst_size
) {
    if ((prev.bitset.hidden         && !next.bitset.hidden)
    ||  (prev.bitset.faint          && !next.bitset.faint)
    ||  (prev.bitset.italic         && !next.bitset.italic)
    ||  (prev.bitset.underline      && !next.bitset.underline)
    ||  (prev.bitset.blinking       && !next.bitset.blinking)
    ||  (prev.bitset.strikethrough  && !next.bitset.strikethrough)
    ||  (prev.bitset.fg             && !next.bitset.fg)
    ||  (prev.bitset.bg             && !next.bitset.bg)) {
        struct amp_style_type style = next;

        style.bitset.reset = true;

        return amp_style_to_ans(style, pal, ans_dst, ans_dst_size);
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
        },
        {
            .fg = next.fg,
            .bg = next.bg,
            .bitset = {
                .fg = (
                    !prev.bitset.fg && next.bitset.fg
                ),
                .bg = (
                    !prev.bitset.bg && next.bitset.bg
                )
            }
        }
    };

    for (size_t i=0; i<sizeof(styles)/sizeof(styles[0]); ++i) {
        size_t buf_size = amp_style_to_ans(styles[i], pal, buf, sizeof(buf));

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
            prev_style_state, next_style_state, amp->palette,
            style_ans, sizeof(style_ans)
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

static inline struct amp_rgb16_type amp_find_rgb16(
    const struct amp_rgb16_type *table, struct amp_rgb_type rgb
) {
    long best_d = LONG_MAX;
    const struct amp_rgb16_type *best_row = nullptr;

    for (; table->code; ++table) {
        if (table->index == AMP_COLOR_NONE) {
            continue;
        }

        long dr = rgb.r;
        long dg = rgb.g;
        long db = rgb.b;

         dr -= table->rgb.r;
         dg -= table->rgb.g;
         db -= table->rgb.b;

         long d = dr * dr + dg * dg + db * db;

         if (d < best_d) {
            best_d = d;
            best_row = table;
         }
    }

    return *best_row;
}

static inline struct amp_rgb_type amp_color_rgb(AMP_COLOR color) {
    return (
        color < AMP_MAX_COLOR ? (
            amp_rgb16_fg_table[color].rgb
        ) : amp_rgb16_fg_table[AMP_COLOR_NONE].rgb
    );
}

#endif
