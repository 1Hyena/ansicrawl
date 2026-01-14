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
#include <ctype.h>
////////////////////////////////////////////////////////////////////////////////


struct amp_type;
struct amp_color_type;
struct amp_mode_type;

static constexpr size_t AMP_CELL_GLYPH_SIZE = 5; // 4 bytes for UTF8 + null byte
static constexpr size_t AMP_CELL_MODE_SIZE  = 7;
static constexpr size_t AMP_CELL_SIZE       = (
    AMP_CELL_GLYPH_SIZE + AMP_CELL_MODE_SIZE
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

struct amp_color_type {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef enum : uint64_t {
    AMP_STYLE_NONE      = 0,
    ////////////////////////////////////////////////////////////////////////////
    AMP_HIDDEN          = (1ULL <<  0), AMP_FAINT           = (1ULL <<  1),
    AMP_ITALIC          = (1ULL <<  2), AMP_UNDERLINE       = (1ULL <<  3),
    AMP_BLINKING        = (1ULL <<  4), AMP_STRIKETHROUGH   = (1ULL <<  5),
    ////////////////////////////////////////////////////////////////////////////
    AMP_FG_NONE         = (1ULL <<  6), AMP_FG_BLACK        = (1ULL <<  7),
    AMP_FG_MAROON       = (1ULL <<  8), AMP_FG_GREEN        = (1ULL <<  9),
    AMP_FG_OLIVE        = (1ULL << 10), AMP_FG_NAVY         = (1ULL << 11),
    AMP_FG_PURPLE       = (1ULL << 12), AMP_FG_TEAL         = (1ULL << 13),
    AMP_FG_SILVER       = (1ULL << 14), AMP_FG_GRAY         = (1ULL << 15),
    AMP_FG_RED          = (1ULL << 16), AMP_FG_LIME         = (1ULL << 17),
    AMP_FG_YELLOW       = (1ULL << 18), AMP_FG_BLUE         = (1ULL << 19),
    AMP_FG_MAGENTA      = (1ULL << 20), AMP_FG_CYAN         = (1ULL << 21),
    AMP_FG_WHITE        = (1ULL << 22),
    ////////////////////////////////////////////////////////////////////////////
    AMP_BG_NONE         = (1ULL << 24), AMP_BG_BLACK        = (1ULL << 25),
    AMP_BG_MAROON       = (1ULL << 26), AMP_BG_GREEN        = (1ULL << 27),
    AMP_BG_OLIVE        = (1ULL << 28), AMP_BG_NAVY         = (1ULL << 29),
    AMP_BG_PURPLE       = (1ULL << 30), AMP_BG_TEAL         = (1ULL << 31),
    AMP_BG_SILVER       = (1ULL << 32), AMP_BG_GRAY         = (1ULL << 33),
    AMP_BG_RED          = (1ULL << 34), AMP_BG_LIME         = (1ULL << 35),
    AMP_BG_YELLOW       = (1ULL << 36), AMP_BG_BLUE         = (1ULL << 37),
    AMP_BG_MAGENTA      = (1ULL << 38), AMP_BG_CYAN         = (1ULL << 39),
    AMP_BG_WHITE        = (1ULL << 40)
} AMP_STYLE;

typedef enum : uint8_t {
    AMP_PAL_RGB16 = 0,
    AMP_PAL_24BIT
} AMP_PALETTE;

typedef enum : uint8_t {
    AMP_ALIGN_LEFT = 0,
    AMP_ALIGN_CENTER,
    AMP_ALIGN_RIGHT
} AMP_ALIGN;

// Public API: /////////////////////////////////////////////////////////////////
static inline size_t                    amp_init(
    struct amp_type *                       amp,
    void *                                  data,
    size_t                                  data_size
);
static inline void                      amp_clear(
    struct amp_type *                       amp
);
static inline void                      amp_set_palette(
    struct amp_type *                       amp,
    AMP_PALETTE                             palette
);
static inline void                      amp_draw_glyph(
    struct amp_type *                       amp,
    AMP_STYLE                               glyph_style,
    long                                    glyph_x,
    long                                    glyph_y,
    const char *                            glyph_str
);
static inline void                      amp_draw_text(
    struct amp_type *                       amp,
    AMP_STYLE                               text_style,
    long                                    text_x,
    long                                    text_y,
    AMP_ALIGN                               text_alignment,
    const char *                            text_str
);
static inline size_t                    amp_draw_multiline_text(
    struct amp_type *                       amp,
    AMP_STYLE                               text_style,
    long                                    text_x,
    long                                    text_y,
    uint32_t                                text_max_width,
    AMP_ALIGN                               text_alignment,
    const char *                            text_str
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
static inline const char *              amp_put_glyph(
    struct amp_type *                       amp,
    const char *                            glyph,
    uint32_t                                x,
    uint32_t                                y
);
static inline const char *              amp_get_glyph(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline AMP_STYLE                 amp_get_style(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y
);
static inline bool                      amp_put_style(
    struct amp_type *                       amp,
    AMP_STYLE                               style,
    uint32_t                                x,
    uint32_t                                y
);
static inline struct amp_color_type     amp_map_rgb(
    uint8_t                                 r,
    uint8_t                                 g,
    uint8_t                                 b
);
static inline void                      amp_unmap_rgb(
    struct amp_color_type                   color,
    uint8_t *                               r,
    uint8_t *                               g,
    uint8_t *                               b
);
static inline struct amp_color_type     amp_lookup_color(
    AMP_COLOR                               index
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
    } mode;

    AMP_PALETTE palette;
};

struct amp_mode_type {
    struct amp_color_type fg;
    struct amp_color_type bg;

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

static const struct {
    struct {
        AMP_STYLE fg;
        AMP_STYLE bg;
    } style;

    struct amp_color_type   color;
    AMP_COLOR               index;
} amp_color_table[] = {
    [AMP_COLOR_NONE] = {
        .index  = AMP_COLOR_NONE,
        .color  = {},
        .style  = { .fg = AMP_FG_NONE, .bg = AMP_BG_NONE }
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_BLACK] = {
        .index  = AMP_BLACK,    .color  = { .r  = 0,    .g  = 0,    .b  = 0   },
        .style  = {
            .fg = AMP_FG_BLACK,
            .bg = AMP_BG_BLACK
        }
    },
    [AMP_MAROON] = {
        .index  = AMP_MAROON,   .color  = { .r  = 128,  .g  = 0,    .b  = 0   },
        .style  = {
            .fg = AMP_FG_MAROON,
            .bg = AMP_BG_MAROON
        }
    },
    [AMP_GREEN] = {
        .index  = AMP_GREEN,    .color  = { .r  = 0,    .g  = 128,  .b  = 0   },
        .style  = {
            .fg = AMP_FG_GREEN,
            .bg = AMP_BG_GREEN
        }
    },
    [AMP_OLIVE] = {
        .index  = AMP_OLIVE,    .color  = { .r  = 128,  .g  = 128,  .b  = 0   },
        .style  = {
            .fg = AMP_FG_OLIVE,
            .bg = AMP_BG_OLIVE
        }
    },
    [AMP_NAVY] = {
        .index  = AMP_NAVY,     .color  = { .r  = 0,    .g  = 0,    .b  = 128 },
        .style  = {
            .fg = AMP_FG_NAVY,
            .bg = AMP_BG_NAVY
        }
    },
    [AMP_PURPLE] = {
        .index  = AMP_PURPLE,   .color  = { .r  = 128,  .g  = 0,    .b  = 128 },
        .style  = {
            .fg = AMP_FG_PURPLE,
            .bg = AMP_BG_PURPLE
        }
    },
    [AMP_TEAL] = {
        .index  = AMP_TEAL,     .color  = { .r  = 0,    .g  = 128,  .b  = 128 },
        .style  = {
            .fg = AMP_FG_TEAL,
            .bg = AMP_BG_TEAL
        }
    },
    [AMP_SILVER] = {
        .index  = AMP_SILVER,   .color  = { .r  = 128,  .g  = 128,  .b  = 128 },
        .style  = {
            .fg = AMP_FG_SILVER,
            .bg = AMP_BG_SILVER
        }
    },
    [AMP_GRAY] = {
        .index  = AMP_GRAY,     .color  = { .r  = 64,   .g  = 64,   .b  = 64  },
        .style  = {
            .fg = AMP_FG_GRAY,
            .bg = AMP_BG_GRAY
        }
    },
    [AMP_RED] = {
        .index  = AMP_RED,      .color  = { .r  = 255,  .g  = 0,    .b  = 0   },
        .style  = {
            .fg = AMP_FG_RED,
            .bg = AMP_BG_RED
        }
    },
    [AMP_LIME] = {
        .index  = AMP_LIME,     .color  = { .r  = 0,    .g  = 255,  .b  = 0   },
        .style  = {
            .fg = AMP_FG_LIME,
            .bg = AMP_BG_LIME
        }
    },
    [AMP_YELLOW] = {
        .index  = AMP_YELLOW,   .color  = { .r  = 255,  .g  = 255,  .b  = 0   },
        .style  = {
            .fg = AMP_FG_YELLOW,
            .bg = AMP_BG_YELLOW
        }
    },
    [AMP_BLUE] = {
        .index  = AMP_BLUE,     .color  = { .r  = 0,    .g  = 0,    .b  = 255 },
        .style  = {
            .fg = AMP_FG_BLUE,
            .bg = AMP_BG_BLUE
        }
    },
    [AMP_MAGENTA] = {
        .index  = AMP_MAGENTA,  .color  = { .r  = 255,  .g  = 0,    .b  = 255 },
        .style  = {
            .fg = AMP_FG_MAGENTA,
            .bg = AMP_BG_MAGENTA
        }
    },
    [AMP_CYAN] = {
        .index  = AMP_CYAN,     .color  = { .r  = 0,    .g  = 255,  .b  = 255 },
        .style  = {
            .fg = AMP_FG_CYAN,
            .bg = AMP_BG_CYAN
        }
    },
    [AMP_WHITE] = {
        .index  = AMP_WHITE,    .color  = { .r  = 255,  .g  = 255,  .b  = 255 },
        .style  = {
            .fg = AMP_FG_WHITE,
            .bg = AMP_BG_WHITE
        }
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_MAX_COLOR] = {}
};

static const struct amp_rgb16_type {
    const char *            code;
    struct amp_color_type   color;
    AMP_COLOR               index;
    bool                    bright:1;
} amp_rgb16_fg_table[] = {
    [AMP_COLOR_NONE] = {
        .index  = AMP_COLOR_NONE,
        .code   = "",
        .color  = amp_color_table[AMP_COLOR_NONE].color
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_BLACK] = {
        .index  = AMP_BLACK,
        .code   = "30",
        .color  = amp_color_table[AMP_BLACK].color
    },
    [AMP_MAROON] = {
        .index  = AMP_MAROON,
        .code   = "31",
        .color  = amp_color_table[AMP_MAROON].color
    },
    [AMP_GREEN] = {
        .index  = AMP_GREEN,
        .code   = "32",
        .color  = amp_color_table[AMP_GREEN].color
    },
    [AMP_OLIVE] = {
        .index  = AMP_OLIVE,
        .code   = "33",
        .color  = amp_color_table[AMP_OLIVE].color
    },
    [AMP_NAVY] = {
        .index  = AMP_NAVY,
        .code   = "34",
        .color  = amp_color_table[AMP_NAVY].color
    },
    [AMP_PURPLE] = {
        .index  = AMP_PURPLE,
        .code   = "35",
        .color  = amp_color_table[AMP_PURPLE].color
    },
    [AMP_TEAL] = {
        .index  = AMP_TEAL,
        .code   = "36",
        .color  = amp_color_table[AMP_TEAL].color
    },
    [AMP_SILVER] = {
        .index  = AMP_SILVER,
        .code   = "37",
        .color  = amp_color_table[AMP_SILVER].color
    },
    [AMP_GRAY] = {
        .index  = AMP_GRAY,
        .code   = "30",
        .color  = amp_color_table[AMP_GRAY].color,
        .bright = true
    },
    [AMP_RED] = {
        .index  = AMP_RED,
        .code   = "31",
        .color  = amp_color_table[AMP_RED].color,
        .bright = true
    },
    [AMP_LIME] = {
        .index  = AMP_LIME,
        .code   = "32",
        .color  = amp_color_table[AMP_LIME].color,
        .bright = true
    },
    [AMP_YELLOW] = {
        .index  = AMP_YELLOW,
        .code   = "33",
        .color  = amp_color_table[AMP_YELLOW].color,
        .bright = true
    },
    [AMP_BLUE] = {
        .index  = AMP_BLUE,
        .code   = "34",
        .color  = amp_color_table[AMP_BLUE].color,
        .bright = true
    },
    [AMP_MAGENTA] = {
        .index  = AMP_MAGENTA,
        .code   = "35",
        .color  = amp_color_table[AMP_MAGENTA].color,
        .bright = true
    },
    [AMP_CYAN] = {
        .index  = AMP_CYAN,
        .code   = "36",
        .color  = amp_color_table[AMP_CYAN].color,
        .bright = true
    },
    [AMP_WHITE] = {
        .index  = AMP_WHITE,
        .code   = "37",
        .color  = amp_color_table[AMP_WHITE].color,
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
        .color  = amp_color_table[AMP_BLACK].color
    },
    [AMP_MAROON] = {
        .index  = AMP_MAROON,
        .code   = "41",
        .color  = amp_color_table[AMP_MAROON].color
    },
    [AMP_GREEN] = {
        .index  = AMP_GREEN,
        .code   = "42",
        .color  = amp_color_table[AMP_GREEN].color
    },
    [AMP_OLIVE] = {
        .index  = AMP_OLIVE,
        .code   = "43",
        .color  = amp_color_table[AMP_OLIVE].color
    },
    [AMP_NAVY] = {
        .index  = AMP_NAVY,
        .code   = "44",
        .color  = amp_color_table[AMP_NAVY].color
    },
    [AMP_PURPLE] = {
        .index  = AMP_PURPLE,
        .code   = "45",
        .color  = amp_color_table[AMP_PURPLE].color
    },
    [AMP_TEAL] = {
        .index  = AMP_TEAL,
        .code   = "46",
        .color  = amp_color_table[AMP_TEAL].color
    },
    [AMP_SILVER] = {
        .index  = AMP_SILVER,
        .code   = "47",
        .color  = amp_color_table[AMP_SILVER].color
    },
    [AMP_GRAY] = {
        .index  = AMP_GRAY,
        .code   = "40",
        .color  = amp_color_table[AMP_GRAY].color,
        .bright = true
    },
    [AMP_RED] = {
        .index  = AMP_RED,
        .code   = "41",
        .color  = amp_color_table[AMP_RED].color,
        .bright = true
    },
    [AMP_LIME] = {
        .index  = AMP_LIME,
        .code   = "42",
        .color  = amp_color_table[AMP_LIME].color,
        .bright = true
    },
    [AMP_YELLOW] = {
        .index  = AMP_YELLOW,
        .code   = "43",
        .color  = amp_color_table[AMP_YELLOW].color,
        .bright = true
    },
    [AMP_BLUE] = {
        .index  = AMP_BLUE,
        .code   = "44",
        .color  = amp_color_table[AMP_BLUE].color,
        .bright = true
    },
    [AMP_MAGENTA] = {
        .index  = AMP_MAGENTA,
        .code   = "45",
        .color  = amp_color_table[AMP_MAGENTA].color,
        .bright = true
    },
    [AMP_CYAN] = {
        .index  = AMP_CYAN,
        .code   = "46",
        .color  = amp_color_table[AMP_CYAN].color,
        .bright = true
    },
    [AMP_WHITE] = {
        .index  = AMP_WHITE,
        .code   = "47",
        .color  = amp_color_table[AMP_WHITE].color,
        .bright = true
    },
    ////////////////////////////////////////////////////////////////////////////
    [AMP_MAX_COLOR] = {}
};

// Private API: ////////////////////////////////////////////////////////////////
static inline bool                      amp_set_mode(
    struct amp_type *                       amp,
    uint32_t                                x,
    uint32_t                                y,
    struct amp_mode_type                    mode
);
static inline struct amp_mode_type      amp_get_mode(
    const struct amp_type *                 amp,
    uint32_t                                x,
    uint32_t                                y
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
static inline size_t                    amp_utf8_code_point_count(
    const char *                            utf8_str,
    size_t                                  utf8_str_size
);
static inline size_t                    amp_mode_to_ans(
    struct amp_mode_type                    mode,
    AMP_PALETTE                             palette,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline size_t                    amp_mode_update_to_ans(
    struct amp_mode_type                    prev_mode,
    struct amp_mode_type                    next_mode,
    AMP_PALETTE                             palette,
    char *                                  ans_dst,
    size_t                                  ans_dst_size
);
static inline struct amp_mode_type     amp_mode_cell_deserialize(
    const uint8_t *                         src,
    size_t                                  src_size
);
static inline bool                      amp_mode_cell_serialize(
    struct amp_mode_type                    mode,
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
    struct amp_color_type                   color
);
static inline const char *              amp_str_seg_first_line_size(
    const char *                            str,
    size_t                                  str_size,
    size_t *                                line_size
);
static inline const char *              amp_str_first_line_size(
    const char *                            str,
    size_t *                                line_size
);
static inline const char *              amp_str_seg_skip_style_sign(
    const char *                            str,
    size_t                                  str_size
);
static inline const char *              amp_str_skip_style_sign(
    const char *                            str
);
static inline const char *              amp_str_seg_skip_utf8_symbol(
    const char *                            str,
    size_t                                  str_size
);
static inline const char *              amp_str_skip_utf8_symbol(
    const char *                            str
);
static inline const char *              amp_str_seg_skip_utf8_symbol_count(
    const char *                            str,
    size_t                                  str_size,
    size_t                                  utf8_symbol_count
);
static inline const char *              amp_str_skip_utf8_symbol_count(
    const char *                            str,
    size_t                                  utf8_symbol_count
);
static inline size_t                    amp_str_seg_width(
    const char *                            str,
    size_t                                  str_size
);
static inline size_t                    amp_str_width(
    const char *                            str
);
static inline void                      amp_draw_text_clip(
    struct amp_type *                       amp,
    AMP_STYLE                               text_style,
    long                                    text_x,
    long                                    text_y,
    AMP_ALIGN                               text_alignment,
    const char *                            text_str,
    size_t                                  text_str_size
);
static inline size_t                    amp_draw_multiline_text_clip(
    struct amp_type *                       amp,
    AMP_STYLE                               text_style,
    long                                    text_x,
    long                                    text_y,
    uint32_t                                text_max_width,
    AMP_ALIGN                               text_alignment,
    const char *                            text_str,
    size_t                                  text_str_size
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
    const size_t mode_size = cell_count * AMP_CELL_MODE_SIZE;

    amp->glyph.data = (uint8_t *) buf;
    amp->glyph.size = glyph_size;

    amp->mode.data = (uint8_t *) buf + amp->glyph.size;
    amp->mode.size = mode_size;

    amp_clear(amp);

    return bytes_required;
}

static inline void amp_clear(struct amp_type *amp) {
    memset(amp->glyph.data, 0, amp->glyph.size);
    memset(amp->mode.data, 0, amp->mode.size);
}

static inline void amp_set_palette(struct amp_type *amp, AMP_PALETTE palette) {
    amp->palette = palette;
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

static inline size_t amp_utf8_code_point_count(
    const char *utf8_str, size_t utf8_str_size
) {
    size_t code_point_count = 0;

    for (const char *s = utf8_str; *s;) {
        int cpsz = amp_utf8_code_point_size(
            s, amp_sub_size(utf8_str_size, (size_t) (s - utf8_str))
        );

        if (cpsz < 0) {
            break;
        }

        ++code_point_count;
        s += cpsz;
    }

    return code_point_count;
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

static inline const char *amp_put_glyph(
    struct amp_type *amp, const char *glyph, uint32_t x, uint32_t y
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

static inline bool amp_set_mode(
    struct amp_type *amp, uint32_t x, uint32_t y,
    struct amp_mode_type mode
) {
    ssize_t cell_index = amp_get_cell_index(amp, x, y);

    if (cell_index < 0) {
        return false;
    }

    if ((size_t) cell_index * AMP_CELL_MODE_SIZE >= amp->mode.size) {
        return false;
    }

    return amp_mode_cell_serialize(
        mode, amp->mode.data + (size_t) cell_index * AMP_CELL_MODE_SIZE,
        AMP_CELL_MODE_SIZE
    );
}

static inline struct amp_mode_type amp_get_mode(
    const struct amp_type *amp, uint32_t x, uint32_t y
) {
    static const struct amp_mode_type broken_cell = {
        .bitset = { .broken = true }
    };

    ssize_t cell_index = amp_get_cell_index(amp, x, y);

    if (cell_index < 0) {
        return broken_cell;
    }

    if ((size_t) cell_index * AMP_CELL_MODE_SIZE >= amp->mode.size) {
        return broken_cell;
    }

    return amp_mode_cell_deserialize(
        amp->mode.data + (size_t) cell_index * AMP_CELL_MODE_SIZE,
        AMP_CELL_MODE_SIZE
    );
}

static inline AMP_STYLE amp_get_style(
    const struct amp_type *amp, uint32_t x, uint32_t y
) {
    auto mode = amp_get_mode(amp, x, y);

    return (
        (mode.bitset.hidden         ? AMP_HIDDEN        : 0) |
        (mode.bitset.faint          ? AMP_FAINT         : 0) |
        (mode.bitset.italic         ? AMP_ITALIC        : 0) |
        (mode.bitset.underline      ? AMP_UNDERLINE     : 0) |
        (mode.bitset.blinking       ? AMP_BLINKING      : 0) |
        (mode.bitset.strikethrough  ? AMP_STRIKETHROUGH : 0)
    );
}

static inline bool amp_put_style(
    struct amp_type *amp, AMP_STYLE style, uint32_t x, uint32_t y
) {
    auto mode = amp_get_mode(amp, x, y);

    constexpr AMP_STYLE bg_colors = (
        AMP_BG_NONE|AMP_BG_BLACK|AMP_BG_MAROON| AMP_BG_GREEN|AMP_BG_OLIVE|
        AMP_BG_NAVY|AMP_BG_PURPLE|AMP_BG_TEAL|AMP_BG_SILVER|AMP_BG_GRAY|
        AMP_BG_RED|AMP_BG_LIME|AMP_BG_YELLOW|AMP_BG_BLUE|AMP_BG_MAGENTA|
        AMP_BG_CYAN|AMP_BG_WHITE
    );

    constexpr AMP_STYLE fg_colors = (
        AMP_FG_NONE|AMP_FG_BLACK|AMP_FG_MAROON| AMP_FG_GREEN|AMP_FG_OLIVE|
        AMP_FG_NAVY|AMP_FG_PURPLE|AMP_FG_TEAL|AMP_FG_SILVER|AMP_FG_GRAY|
        AMP_FG_RED|AMP_FG_LIME|AMP_FG_YELLOW|AMP_FG_BLUE|AMP_FG_MAGENTA|
        AMP_FG_CYAN|AMP_FG_WHITE
    );

    if (style & bg_colors) {
        if (style & AMP_BG_NONE) {
            mode.bitset.bg = false;
        }
        else {
            unsigned bg_r = 0;
            unsigned bg_g = 0;
            unsigned bg_b = 0;
            unsigned bg_colors = 0;

            mode.bitset.bg = true;

            for (AMP_COLOR i = AMP_COLOR_NONE + 1; i < AMP_MAX_COLOR; ++i) {
                auto row = amp_color_table[i];

                if (row.style.bg & style) {
                    bg_r += row.color.r;
                    bg_g += row.color.g;
                    bg_b += row.color.b;

                    ++bg_colors;
                }
            }

            bg_r /= bg_colors;
            bg_g /= bg_colors;
            bg_b /= bg_colors;

            mode.bg = amp_map_rgb(
                (uint8_t) bg_r, (uint8_t) bg_g, (uint8_t) bg_b
            );
        }
    }
    else mode.bitset.bg = false;

    if (style & fg_colors) {
        if (style & AMP_FG_NONE) {
            mode.bitset.fg = false;
        }
        else {
            unsigned fg_r = 0;
            unsigned fg_g = 0;
            unsigned fg_b = 0;
            unsigned fg_colors = 0;

            mode.bitset.fg = true;

            for (AMP_COLOR i = AMP_COLOR_NONE + 1; i < AMP_MAX_COLOR; ++i) {
                auto row = amp_color_table[i];

                if (row.style.fg & style) {
                    fg_r += row.color.r;
                    fg_g += row.color.g;
                    fg_b += row.color.b;

                    ++fg_colors;
                }
            }

            fg_r /= fg_colors;
            fg_g /= fg_colors;
            fg_b /= fg_colors;

            mode.fg = amp_map_rgb(
                (uint8_t) fg_r, (uint8_t) fg_g, (uint8_t) fg_b
            );
        }
    }
    else mode.bitset.fg = false;

    mode.bitset.hidden          = style & AMP_HIDDEN;
    mode.bitset.faint           = style & AMP_FAINT;
    mode.bitset.italic          = style & AMP_ITALIC;
    mode.bitset.underline       = style & AMP_UNDERLINE;
    mode.bitset.blinking        = style & AMP_BLINKING;
    mode.bitset.strikethrough   = style & AMP_STRIKETHROUGH;

    return amp_set_mode(amp, x, y, mode);
}

static inline void amp_draw_glyph(
    struct amp_type *amp, AMP_STYLE style, long x, long y, const char *glyph_str
) {
    if (x < 0 || x >= amp->width || y < 0 || y >= amp->height
    ||  x > UINT32_MAX || y > UINT32_MAX) {
        return;
    }

    char glyph[AMP_CELL_GLYPH_SIZE] = {};
    size_t glyph_size = 0;

    for (const char *c = glyph_str; *c; ++c) {
        if (++glyph_size >= sizeof(glyph)) {
            break;
        }
    }

    int cpsz = amp_utf8_code_point_size(glyph_str, glyph_size);

    if (cpsz < 0 || cpsz >= (int) sizeof(glyph)) {
        return;
    }

    memcpy(glyph, glyph_str, (size_t) cpsz);

    if (cpsz == 1 && !isprint(glyph[0])) {
        glyph[0] = '?';
    }

    amp_put_glyph(amp, glyph, (uint32_t) x, (uint32_t) y);
    amp_put_style(amp, style, (uint32_t) x, (uint32_t) y);
}

static inline void amp_draw_text_clip(
    struct amp_type *amp, AMP_STYLE style, long x, long y, AMP_ALIGN align,
    const char *text, size_t text_size
) {
    if (y < 0 || y >= amp->height || y > UINT32_MAX) {
        return;
    }

    uint32_t text_width = (uint32_t) amp_utf8_code_point_count(text, text_size);

    if (align == AMP_ALIGN_RIGHT) {
        x = (x - text_width) + 1;
    }
    else if (align == AMP_ALIGN_CENTER) {
        x -= text_width / 2;
    }

    for (const char *s = text; *s;) {
        int cpsz = amp_utf8_code_point_size(
            s, amp_sub_size(text_size, (size_t) (s - text))
        );

        if (cpsz < 0) {
            break;
        }

        amp_draw_glyph(amp, style, x, y, s);

        if (++x >= amp->width) {
            break;
        }

        s += cpsz;
    }
}

static inline void amp_draw_text(
    struct amp_type *amp, AMP_STYLE style, long x, long y, AMP_ALIGN align,
    const char *text
) {
    amp_draw_text_clip(amp, style, x, y, align, text, strlen(text));
}

static inline size_t amp_draw_multiline_text_clip(
    struct amp_type *amp, AMP_STYLE text_style, long text_x, long text_y,
    uint32_t text_max_width, AMP_ALIGN text_alignment, const char *text_str,
    size_t text_str_size
) {
    size_t line_count = 0;
    const char *line = text_str;
    long y = 0;

    while (*line) {
        size_t linesz;
        const char *next_line = amp_str_seg_first_line_size(
            line, amp_sub_size(text_str_size, (size_t) (line - text_str)),
            &linesz
        );

        const size_t line_width = amp_str_seg_width(line, linesz);

        if (line_width > text_max_width) {
            const char *clip_end = amp_str_seg_skip_utf8_symbol_count(
                line, linesz, text_max_width
            );

            const size_t clip_size = (size_t) (clip_end - line);

            amp_draw_text_clip(
                amp, text_style, text_x, text_y + y, text_alignment,
                line, clip_size
            );

            ++y;
            ++line_count;

            if (clip_size < linesz) {
                size_t extra_lines = amp_draw_multiline_text_clip(
                    amp, text_style, text_x, text_y + y, text_max_width,
                    text_alignment, clip_end, amp_sub_size(linesz, clip_size)
                );

                y += (long) extra_lines;
                line_count += extra_lines;
            }
        }
        else {
            amp_draw_text_clip(
                amp, text_style, text_x, text_y + y, text_alignment,
                line, linesz
            );

            ++y;
            ++line_count;
        }

        if (next_line <= line) {
            break;
        }

        line = next_line;
    }

    return line_count;
}

static inline size_t amp_draw_multiline_text(
    struct amp_type *amp, AMP_STYLE text_style, long text_x, long text_y,
    uint32_t text_max_width, AMP_ALIGN text_alignment, const char *text_str
) {
    return amp_draw_multiline_text_clip(
        amp, text_style, text_x, text_y, text_max_width, text_alignment,
        text_str, strlen(text_str)
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

static inline size_t amp_mode_to_ans(
    struct amp_mode_type mode, AMP_PALETTE pal,
    char *ans_dst, size_t ans_dst_size
) {
    char ans[256];
    size_t ans_size = 0;

    const char *options[] = {
        mode.bitset.reset           ? "0" : nullptr,
        mode.bitset.hidden          ? "8" : nullptr,
        mode.bitset.faint           ? "2" : nullptr,
        mode.bitset.italic          ? "3" : nullptr,
        mode.bitset.underline       ? "4" : nullptr,
        mode.bitset.blinking        ? "5" : nullptr,
        mode.bitset.strikethrough   ? "9" : nullptr
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
        if (mode.bitset.fg) {
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
                amp_number_table[mode.fg.r]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[mode.fg.g]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[mode.fg.b]
            );
        }

        if (mode.bitset.bg) {
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
                amp_number_table[mode.bg.r]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[mode.bg.g]
            );

            ans_size += (
                ans_size ? amp_str_append(
                    ans + ans_size, amp_sub_size(sizeof(ans), ans_size), ";"
                ) : 0
            );

            ans_size += amp_str_append(
                ans + ans_size, amp_sub_size(sizeof(ans), ans_size),
                amp_number_table[mode.bg.b]
            );
        }
    }
    else {
        if (mode.bitset.bg) {
            auto bg_rgb_row = amp_find_rgb16(amp_rgb16_bg_table, mode.bg);

            if (bg_rgb_row.bright) {
                struct amp_color_type buf = mode.bg;
                mode.bg = mode.fg;
                mode.fg = buf;
                mode.bitset.bg = mode.bitset.fg;
                mode.bitset.fg = true;

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

        if (mode.bitset.fg) {
            auto fg_rgb_row = amp_find_rgb16(amp_rgb16_fg_table, mode.fg);

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

        if (mode.bitset.bg) {
            auto bg_rgb_row = amp_find_rgb16(amp_rgb16_bg_table, mode.bg);

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

static inline size_t amp_mode_update_to_ans(
    struct amp_mode_type prev, struct amp_mode_type next, AMP_PALETTE pal,
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
        struct amp_mode_type mode = next;

        mode.bitset.reset = true;

        return amp_mode_to_ans(mode, pal, ans_dst, ans_dst_size);
    }

    char buf[256];
    char ans[256];
    size_t ans_size = 0;

    const struct amp_mode_type modes[] = {
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

    for (size_t i=0; i<sizeof(modes)/sizeof(modes[0]); ++i) {
        size_t buf_size = amp_mode_to_ans(modes[i], pal, buf, sizeof(buf));

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

    char mode_ans[256];
    struct amp_mode_type prev_mode_state = {};
    size_t ans_size = 0;

    for (; x < end_x; ++x) {
        struct amp_mode_type next_mode_state = amp_get_mode(amp, x, y);

        size_t mode_ans_size = amp_mode_update_to_ans(
            prev_mode_state, next_mode_state, amp->palette,
            mode_ans, sizeof(mode_ans)
        );

        prev_mode_state = next_mode_state;

        if (mode_ans_size) {
            ans_size += amp_str_append(
                ans_dst + ans_size, amp_sub_size(ans_dst_size, ans_size),
                mode_ans
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

static inline struct amp_mode_type amp_mode_cell_deserialize(
    const uint8_t *data, size_t data_size
) {
    return (struct amp_mode_type) {
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
            .broken         = data_size < AMP_CELL_MODE_SIZE
        }
    };
}

static inline bool amp_mode_cell_serialize(
    struct amp_mode_type cell, uint8_t *dst, size_t dst_size
) {
    if (dst_size < AMP_CELL_MODE_SIZE) {
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
    const struct amp_rgb16_type *table, struct amp_color_type color
) {
    long best_d = LONG_MAX;
    const struct amp_rgb16_type *best_row = nullptr;

    for (; table->code; ++table) {
        if (table->index == AMP_COLOR_NONE) {
            continue;
        }

        long dr = color.r;
        long dg = color.g;
        long db = color.b;

         dr -= table->color.r;
         dg -= table->color.g;
         db -= table->color.b;

         long d = dr * dr + dg * dg + db * db;

         if (d < best_d) {
            best_d = d;
            best_row = table;
         }
    }

    return *best_row;
}

static inline struct amp_color_type amp_lookup_color(AMP_COLOR index) {
    return (
        index < AMP_MAX_COLOR ? (
            amp_color_table[index].color
        ) : amp_color_table[AMP_COLOR_NONE].color
    );
}

static inline struct amp_color_type amp_map_rgb(
    uint8_t r, uint8_t g, uint8_t b
) {
    return (struct amp_color_type) { .r = r, .g = g, .b = b };
}

static inline void amp_unmap_rgb(
    struct amp_color_type color, uint8_t * r, uint8_t * g, uint8_t * b
) {
    *r = color.r;
    *g = color.g;
    *b = color.b;
}

static inline const char *amp_str_seg_first_line_size(
    const char *str, size_t str_sz, size_t *line_size
) {
    const char *begin = str;
    const char *found = nullptr;
    char after_found;

    const char *s = str;

    while (*s && s < str + str_sz) {
        if (*s == '\n') {
            found = s;
            after_found = '\r';
            break;
        }
        else if (*s == '\r') {
            found = s;
            after_found = '\n';
            break;
        }

        ++s;
    }

    if (found) {
        if (*(s + 1) == after_found) {
            s += 2;
        }
        else {
            s += 1;
        }
    }

    if (line_size) {
        *line_size = found ? (size_t) (found - begin) : (size_t) (s - begin);
    }

    return s;
}

static inline const char *amp_str_first_line_size(
    const char *str, size_t *line_size
) {
    return amp_str_seg_first_line_size(str, strlen(str), line_size);
}

static inline const char *amp_str_seg_skip_style_sign(
    const char *str, size_t str_sz
) {
    const char *s = str;
    bool style = false;

    while (*s && s < str + str_sz) {
        if (*s == '{' && !style) {
            style = true;
        }
        else if (style) {
            if (*s == '{') {
                return s;
            }

            const char *end = amp_str_seg_skip_utf8_symbol(
                s, str_sz - (size_t) (s - str)
            );

            return end > s ? end : str;
        }
        else break;

        ++s;
    }

    return str;
}

static inline const char *amp_str_skip_style_sign(const char *str) {
    return amp_str_seg_skip_style_sign(str, strlen(str));
}

static inline const char *amp_str_seg_skip_utf8_symbol(
    const char *str, size_t str_sz
) {
    if (!str_sz) {
        return str;
    }

    int sz = amp_utf8_code_point_size(str, str_sz);

    if (sz == 0) {
        return str;
    }
    else if (sz < 0) {
        return str + 1;
    }

    return str + sz;
}

static inline const char *amp_str_skip_utf8_symbol(const char *str) {
    return amp_str_seg_skip_utf8_symbol(str, strlen(str));
}

static inline size_t amp_str_seg_width(const char *str, size_t str_sz) {
    const char *s = str;
    size_t width = 0;

    while (*s && s < str + str_sz) {
        size_t linesz;
        const char *next_line = amp_str_seg_first_line_size(
            s, str_sz - (size_t) (s - str), &linesz
        );

        size_t line_width = amp_utf8_code_point_count(s, linesz);

        width = width > line_width ? width : line_width;

        if (next_line <= s) {
            break;
        }

        s = next_line;
    }

    return width;
}

static inline size_t amp_str_width(const char *str) {
    return amp_str_seg_width(str, strlen(str));
}

static inline const char *amp_str_seg_skip_utf8_symbol_count(
    const char *str, size_t str_size, size_t utf8_symbol_count
) {
    const char *s = str;
    size_t count = 0;

    while (*s && s < str + str_size) {
        const char *next = amp_str_seg_skip_utf8_symbol(
            s, str_size - (size_t) (s - str)
        );

        if (next <= s) {
            break;
        }

        s = next;

        if (++count >= utf8_symbol_count) {
            break;
        }
    }

    return s;
}

static inline const char *amp_str_skip_utf8_symbol_count(
    const char *str, size_t symbol_count
) {
    return amp_str_seg_skip_utf8_symbol_count(str, strlen(str), symbol_count);
}

#endif
