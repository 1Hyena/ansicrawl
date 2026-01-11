// SPDX-License-Identifier: MIT
#ifndef DISPATCHER_H_11_01_2026
#define DISPATCHER_H_11_01_2026
////////////////////////////////////////////////////////////////////////////////
#include "global.h"
////////////////////////////////////////////////////////////////////////////////

struct DISPATCHER {
    struct {
        struct {
            struct {
                CLIP *clip;
            } incoming;

            struct {
                CLIP *clip;
            } outgoing;
        } terminal;

        struct {
            struct {
                CLIP *clip;
            } incoming;

            struct {
                CLIP *clip;
            } outgoing;
        } client;
    } io;
};

DISPATCHER *dispatcher_create();
void        dispatcher_destroy(DISPATCHER *);
void        dispatcher_init(DISPATCHER *);
void        dispatcher_deinit(DISPATCHER *);
bool        dispatcher_update(DISPATCHER *);

#endif
