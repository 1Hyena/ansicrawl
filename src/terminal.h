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

typedef enum : unsigned char {
    TERMINAL_STATE_NONE = 0,
    ////////////////////////////////////////////////////////////////////////////
    TERMINAL_IDLE,
    TERMINAL_INIT_EDITOR,
    TERMINAL_ASK_SCREEN_SIZE,
    TERMINAL_GET_SCREEN_SIZE,
    ////////////////////////////////////////////////////////////////////////////
    MAX_TERMINAL_STATE
} TERMINAL_STATE;

struct TERMINAL {
    struct termios original;

    struct {
        struct {
            struct {
                CLIP *clip;
            } incoming;

            struct {
                CLIP *clip;
            } outgoing;
        } interface;

        struct {
            struct {
                CLIP *clip;
            } incoming;

            struct {
                CLIP *clip;
            } outgoing;
        } client;
    } io;

    struct {
        int cx;
        int cy;
        int width;
        int height;
    } screen;

    TERMINAL_STATE state;

    struct {
        struct {
            struct {
                bool recv_will:1;
                bool sent_do:1;

                struct {
                    uint16_t width;
                    uint16_t height;
                } state;
            } naws;
        } client;
    } telopt;

    struct {
        bool broken:1;
        bool raw:1;
        bool reformat:1;
        bool redraw:1;
    } bitset;
};

TERMINAL *  terminal_create();
void        terminal_destroy(TERMINAL *);
void        terminal_init(TERMINAL *);
void        terminal_deinit(TERMINAL *);
bool        terminal_update(TERMINAL *);
bool        terminal_write_to_interface(TERMINAL *, const char *, size_t len);
bool        terminal_write_to_client(TERMINAL *, const char *, size_t len);
bool        terminal_read_from_client(TERMINAL *);
bool        terminal_flush_outgoing(TERMINAL *);
bool        terminal_fetch_incoming(TERMINAL *);

void terminal_handle_incoming_client_iac(
    TERMINAL *, const uint8_t *data, size_t sz
);

#endif
