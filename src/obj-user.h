// SPDX-License-Identifier: MIT
#ifndef OBJ_USER_H_06_01_2026
#define OBJ_USER_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////


struct USER {
    struct {
        struct {
            CLIP *clip;
        } incoming;

        struct {
            CLIP *clip;
        } outgoing;
    } io;
};

USER *  user_create();
void    user_destroy(USER *);

#endif
