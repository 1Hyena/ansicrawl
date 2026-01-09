// SPDX-License-Identifier: MIT
#ifndef GLOBAL_H_06_01_2026
#define GLOBAL_H_06_01_2026
////////////////////////////////////////////////////////////////////////////////
#include <signal.h>
#include <limits.h>
////////////////////////////////////////////////////////////////////////////////


#define BREAKPOINT __asm("int $3") // debugging break point

#define MAX_STACKBUF_SIZE   ( PTHREAD_STACK_MIN / 2 )
#define MAX_KEY_HASH        1024
#define MAX_STRING_LENGTH   4608
#define MAX_INPUT_LENGTH    256

typedef struct MEM      MEM;
typedef struct USER     USER;
typedef struct CLIENT   CLIENT;
typedef struct SERVER   SERVER;
typedef struct CLIP     CLIP;
typedef struct TERMINAL TERMINAL;

struct global_type {
    struct {
        volatile sig_atomic_t interrupt;
        volatile sig_atomic_t terminate;
        volatile sig_atomic_t alarm;
        volatile sig_atomic_t pipe;
        volatile sig_atomic_t quit;
        volatile sig_atomic_t window;
    } signal;

    struct {
        MEM *memory;
    } list;

    struct {
        MEM *memory[32][8];
    } free;

    struct {
        struct {
            CLIP *clip;
        } incoming;

        struct {
            CLIP *clip;
        } outgoing;
    } io;

    struct {
        struct timespec boot;
        struct timespec update;
    } time;

    struct {
        size_t update;
    } count;

    TERMINAL *terminal;
    CLIP *logbuf;
    SERVER *server;
    CLIENT *client;

    struct {
        bool shutdown:1;
        bool broken:1;
    } bitset;
};

extern struct global_type global;

#endif
