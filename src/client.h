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
        size_t width;
        size_t height;
    } screen;

    struct {
        struct {
            struct {
                bool sent_will:1;
                bool sent_wont:1;
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
bool    client_read_from_terminal(CLIENT *client);
bool    client_write_to_terminal(CLIENT *, const char *str, size_t len);
void    client_flush_outgoing(CLIENT *);
void    client_fetch_incoming(CLIENT *);

void client_handle_incoming_terminal_iac(
    CLIENT *, const uint8_t *data, size_t sz
);

#endif
