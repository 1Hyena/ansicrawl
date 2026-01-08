// SPDX-License-Identifier: MIT
#ifndef SERVER_H_06_01_2026
#define SERVER_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////


struct SERVER {
    struct {
        struct {
            CLIP *clip;
        } incoming;

        struct {
            CLIP *clip;
        } outgoing;
    } io;
};

SERVER *server_create();
void    server_destroy(SERVER *);

#endif
