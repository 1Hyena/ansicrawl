// SPDX-License-Identifier: MIT
#ifndef CLIENT_H_06_01_2026
#define CLIENT_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////


struct CLIENT {
    struct {
        struct {
            struct {
                CLIP *clip;
            } incoming;

            struct {
                CLIP *clip;
            } outgoing;
        } terminal;
    } io;

    struct {
        size_t width;
        size_t height;
    } screen;

    struct {
        struct {
            struct {
                bool sent_will:1;
                bool recv_do:1;
                bool recv_dont:1;
            } naws;
        } terminal;
    } telopt;
};

CLIENT *client_create();
void    client_destroy(CLIENT *);
void    client_init(CLIENT *);
void    client_deinit(CLIENT *);
void    client_pulse(CLIENT *);
bool    client_write_to_terminal(CLIENT *, const char *str, size_t len);
void    client_flush(CLIENT *);

#endif
