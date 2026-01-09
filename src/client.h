// SPDX-License-Identifier: MIT
#ifndef CLIENT_H_06_01_2026
#define CLIENT_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
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
        size_t  width;
        size_t  height;
        CLIP *  clip;
        size_t  hash;
    } screen;

    struct {
        struct {
            struct {
                bool sent_do:1;
                bool sent_dont:1;
                bool recv_will:1;
                bool recv_wont:1;
            } naws;
        } terminal;
    } telopt;

    struct {
        bool reformat:1;
        bool redraw:1;
    } bitset;
};

CLIENT *client_create();
void    client_destroy(CLIENT *);
void    client_init(CLIENT *);
void    client_deinit(CLIENT *);
bool    client_update(CLIENT *);
bool    client_read_from_terminal(CLIENT *client);
bool    client_write_to_terminal(CLIENT *, const char *str, size_t len);
bool    client_flush_outgoing(CLIENT *);
bool    client_fetch_incoming(CLIENT *);

void client_handle_incoming_terminal_iac(
    CLIENT *, const uint8_t *data, size_t sz
);

#endif
