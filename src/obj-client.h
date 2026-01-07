// SPDX-License-Identifier: MIT
#ifndef OBJ_CLIENT_H_06_01_2026
#define OBJ_CLIENT_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////
#include <termios.h>
////////////////////////////////////////////////////////////////////////////////


struct CLIENT {
    struct {
        struct {
            CLIP *clip;
        } incoming;

        struct {
            CLIP *clip;
        } outgoing;
    } io;

    struct {
      int cx, cy;
      int screenrows;
      int screencols;
      struct termios orig_termios;
    } tui;

    struct {
        bool broken:1;
    } bitset;
};

CLIENT *client_create();
void    client_destroy(CLIENT *);
void    client_init(CLIENT *);
void    client_deinit(CLIENT *);
void    client_pulse(CLIENT *);
bool    client_write(CLIENT *, const char *str, size_t len);

#endif
