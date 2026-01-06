// SPDX-License-Identifier: MIT
#ifndef TERMINAL_H_06_01_2026
#define TERMINAL_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
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

#endif
