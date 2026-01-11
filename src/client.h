// SPDX-License-Identifier: MIT
#ifndef CLIENT_H_06_01_2026
#define CLIENT_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
#include "telnet.h"
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
                size_t total;
            } outgoing;
        } terminal;

        struct {
            struct {
                CLIP *clip;
            } incoming;

            struct {
                CLIP *clip;
            } outgoing;
        } dispatcher;
    } io;

    struct {
        size_t  width;
        size_t  height;
        CLIP *  clip;
        size_t  hash;
    } screen;

    struct {
        struct {
            struct telnet_opt_type naws;
            struct telnet_opt_type echo;
            struct telnet_opt_type sga;
            struct telnet_opt_type bin;
        } terminal;
    } telopt;

    struct {
        bool shutdown:1;
        bool reformat:1;
        bool redraw:1;
    } bitset;
};

CLIENT *client_create();
void    client_destroy(CLIENT *);
void    client_init(CLIENT *);
void    client_deinit(CLIENT *);
bool    client_update(CLIENT *);

#endif
