// SPDX-License-Identifier: MIT
#ifndef TERMINAL_H_06_01_2026
#define TERMINAL_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////
//#include <stddef.h>
#include <termios.h>
////////////////////////////////////////////////////////////////////////////////


static constexpr size_t TERMINAL_DEFAULT_WIDTH          = 80;
static constexpr size_t TERMINAL_DEFAULT_HEIGHT         = 24;
static constexpr char   TERMINAL_ESC_HIDDEN[]           = "\033[8m";
static constexpr char   TERMINAL_ESC_HIDDEN_RESET[]     = "\033[28m";
static constexpr char   TERMINAL_ESC_BOLD[]             = "\033[1m";
static constexpr char   TERMINAL_ESC_FAINT[]            = "\033[2m";
static constexpr char   TERMINAL_ESC_ITALIC[]           = "\033[3m";
static constexpr char   TERMINAL_ESC_UNDERLINE[]        = "\033[4m";
static constexpr char   TERMINAL_ESC_BLINKING[]         = "\033[5m";
static constexpr char   TERMINAL_ESC_REVERSE[]          = "\033[7m";
static constexpr char   TERMINAL_ESC_STRIKETHROUGH[]    = "\033[9m";
static constexpr char   TERMINAL_ESC_RESET[]            = "\033[0m";
static constexpr char   TERMINAL_ESC_CLEAR_SCREEN[]     = "\x1b[H\x1b[2J";

struct TERMINAL {
    struct termios original;

    struct {
        struct {
            CLIP *clip;
        } incoming;

        struct {
            CLIP *clip;
        } outgoing;
    } io;

    struct {
        int cx;
        int cy;
        int width;
        int height;
    } screen;

    struct {
        bool broken:1;
        bool raw:1;
    } bitset;
};

TERMINAL *  terminal_create();
void        terminal_destroy(TERMINAL *);
void        terminal_init(TERMINAL *);
void        terminal_deinit(TERMINAL *);
void        terminal_pulse(TERMINAL *);
bool        terminal_write(TERMINAL *, const char *str, size_t len);

#endif
