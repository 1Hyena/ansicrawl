// SPDX-License-Identifier: MIT
#ifndef OBJ_CLIENT_H_06_01_2026
#define OBJ_CLIENT_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
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
};

CLIENT *client_create();
void    client_destroy(CLIENT *);
void    client_init(CLIENT *);
void    client_deinit(CLIENT *);
void    client_pulse(CLIENT *);

#endif
